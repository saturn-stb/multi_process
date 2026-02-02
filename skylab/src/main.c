/******************************************************************************
*
* main.c
*
* Description	:
*
* Author		: 
* First Created : 2026.01.31
*
* Copyleft (c) 2026 Every Rights Released.
* All Rights Reversed. 누구나 자유롭게 사용, 수정 및 배포할 수 있습니다.
* 이 소프트웨어는 공유의 가치를 위해 조건 없이 제공됩니다.
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>		 /* For mode constants */
#include <semaphore.h>

#include "msg_def.h"
#include "msgQ.h"
#include "util.h"

/******************************************************************************
*
*
*
*****************************************************************************/
/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
#define MAX_TASK 4 // except SKYLAB, taskParent, taskChild1, taskChild2, taskChild3

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
// 프로세스 정보 구조체
typedef struct 
{
	pid_t pid;
	char name[20];
	int is_running; // 1: Running, 0: Paused

} ProcInfo;

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static int shm_fd = -1;
static ShmQueue *shm_Queue = NULL;
static sem_t *sem_m2p = NULL;
static sem_t *sem_p2m = NULL;
static sem_t *sem_mutex = NULL;

static pid_t p_pid, c1_pid, c2_pid, c3_pid;
static ProcInfo procs[MAX_TASK]; // taskParent, taskChild1, taskChild2, taskChild3

/******************************************************************************
*
*
*
*****************************************************************************/
/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void print_help(void) 
{
	Print("--- Control Commands ---\n" 
			"1. pause  [taskParent/taskChild1/taskChild2/taskChild3]\n"
			"2. resume [taskParent/taskChild1/taskChild2/taskChild3]\n"
			"4. status\n"
			"5. help\n"
			"6. exit\n"
			"7. [Any text for IPC]\n------------------------\n");
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void print_status(void) 
{
	int i = 0;
	Print("\n--- Process Status Report ---\n");
	for (i = 0; i < MAX_TASK; i++) 
	{
		Print("Name: %-10s | PID: %-6d | Status: %s\n", 
				procs[i].name, procs[i].pid, procs[i].is_running ? "RUNNING" : "PAUSED");
	}
	Print("-----------------------------\n");
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void control_process(const char* command, const char* target) 
{
	int i = 0;
	pid_t target_pid = 0;
	
	if (strcmp(target, "taskParent") == 0) target_pid = p_pid;
	else if (strcmp(target, "taskChild1") == 0) target_pid = c1_pid;
	else if (strcmp(target, "taskChild2") == 0) target_pid = c2_pid;
	else if (strcmp(target, "taskChild3") == 0) target_pid = c3_pid;

	if (target_pid == 0) 
	{
		Print("[Main] Unknown target: %s\n", target);
		return;
	}

	for (i = 0; i < MAX_TASK; i++) 
	{
		if (strcmp(procs[i].name, target) == 0) 
		{
			if (strcmp(command, "pause") == 0) 
			{
				kill(target_pid, SIGSTOP);
				procs[i].is_running = 0;
				Print("[Main] %s PAUSED\n", target);
			} 
			else if (strcmp(command, "resume") == 0) 
			{
				kill(procs[i].pid, SIGCONT);
				procs[i].is_running = 1;
				Print("[Main] %s RESUMED\n", target);
			}
		}
	}
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void cleanup_and_exit(int sig) 
{
	(void)sig;
	Print("\n[Main] Terminating all processes...\n");
	kill(p_pid, SIGKILL); 
	kill(c1_pid, SIGKILL); 
	kill(c2_pid, SIGKILL);
	kill(c3_pid, SIGKILL);
	exit(0);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void* send_thread_func(void* arg)
{
	char input[MSG_SIZE];
	unsigned char msg_data[MSG_SIZE];

	(void)arg;

	while (1) 
	{
		Print("Command > ");
		fflush(stdout);

		// 1. 입력 버퍼 초기화 (쓰레기 값 방지)
		memset(input, 0, MSG_SIZE);
		memset(msg_data, 0, MSG_SIZE);		

		if (fgets(input, MSG_SIZE, stdin) != NULL) 
		{
			// 2. 개행 문자(\n) 찾아 NULL(\0)로 치환
			size_t len = strlen(input);
			if (len > 0 && input[len - 1] == '\n') 
			{
				input[len - 1] = '\0';
			} 
			else 
			{
				// 버퍼를 꽉 채워 입력받아 개행 문자가 없는 경우 (입력 초과)
				// 입력 버퍼(stdin)를 비워주지 않으면 다음 fgets 시 남은 데이터가 읽힘
				int c;
				while ((c = getchar()) != '\n' && c != EOF);
			}

			// 3. 빈 문자열 체크 (엔터만 친 경우)
			if (strlen(input) == 0) continue;

			// 1. exit 입력 시 즉시 종료 함수 호출
			if (strcmp(input, "exit") == 0) 
			{
				cleanup_and_exit(SIGINT); // 시그널 핸들러를 수동 호출하여 즉시 종료
			}

			// 2. pause/resume 제어
			char cmd[10], target[20];
			if (sscanf(input, "%s %s", cmd, target) == 2)
			{
				if (strcmp(cmd, "pause") == 0 || strcmp(cmd, "resume") == 0) 
				{
					control_process(cmd, target);
					continue;
				}
			}

			// 3. status 입력 시 status 함수 호출
			if (strcmp(input, "status") == 0) 
			{
				print_status();
				continue;
			}

			// 4. help 입력 시 help 함수 호출
			if (strcmp(input, "help") == 0) 
			{
				print_help();
				continue;
			}

			// --- 공유 메모리 작업 시작 ---
			sem_wait(sem_mutex);

			if(shm_Queue != NULL)
			{			
				// Queue Full 체크
				if (((shm_Queue->head + 1) % QUEUE_SIZE) == shm_Queue->tail) 
				{
					Print("\n[SKYLAB-SEND] Queue Full, waiting...\n");
					sem_post(sem_mutex);
					continue;
				}
				
				// 데이터 쓰기
	            int pos = shm_Queue->head;
				shm_Queue->jobs[pos].proc = PROC_ID_PARENT;
				shm_Queue->jobs[pos].from_proc = PROC_ID_SKYLAB;
				memcpy(shm_Queue->jobs[pos].data, input, MSG_SIZE);
				shm_Queue->head = (pos + 1) % QUEUE_SIZE;
				
				Print("\n[SKYLAB-SEND] %d Queued: %s\n", getpid(), input);

				sem_post(sem_mutex);
				sem_post(sem_m2p); // Parent에게 알림
			}
			else
			{
	            sem_post(sem_mutex);
			}
		}
	}

	return NULL;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void* recv_thread_func(void* arg) 
{
	unsigned char msg_data[MSG_SIZE];

	(void)arg;

	while (1) 
	{
		memset(msg_data, 0, MSG_SIZE);

        // 1. 부모로부터의 신호 대기 (이게 먼저 와야 함)
        sem_wait(sem_p2m);
        // 2. 임계 구역 진입
        sem_wait(sem_mutex);

		if(shm_Queue != NULL)
		{
            // 3. 큐가 비어있는지 확인
	        if (shm_Queue->head == shm_Queue->tail) 
			{
				Print("\n[SKYLAB-RECV] Queue Full! Dropping.\n");
	            sem_post(sem_mutex);
	            continue;
	        }

			// 4. My 메시지 인지 확인
			int proc_id = shm_Queue->jobs[shm_Queue->tail].proc;
			int from_proc_id = shm_Queue->jobs[shm_Queue->tail].from_proc;
			if((proc_id != PROC_ID_SKYLAB) || (from_proc_id != PROC_ID_PARENT))
			{
				Print("\n[SKYLAB-RECV] ID mismatch!!! (0x%02x)\n", proc_id);

				// 내 메시지가 아닌 경우: 락을 풀고 세마포어를 다시 올려서 다른 프로세스가 보게 함
	            sem_post(sem_mutex);
				sem_post(sem_m2p);

				// CPU 점유율 과다 방지를 위한 미세 대기 (Spin-lock 방지)
				DelayUs(100);
	            continue;
			}

			// 5. 데이터 읽기 (Consumer: tail 사용)
			int pos = shm_Queue->tail;
	        shm_Queue->tail = (pos + 1) % QUEUE_SIZE;

	        memcpy(msg_data, shm_Queue->jobs[pos].data, MSG_SIZE);
	        Print("\n[SKYLAB-RECV] Result: %s\n", msg_data);

	        sem_post(sem_mutex);
		}
		else
		{
			sem_post(sem_mutex);
		}
    }

	return NULL;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int main(void) 
{
	signal(SIGINT, cleanup_and_exit);

	pthread_t send_tid, recv_tid;

	// 1. 공유 메모리 오픈 (O_CREAT 추가하여 없으면 생성)
	shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
	if (shm_fd == -1)
	{
		perror("shm_open failed");
		exit(1);
	}

	// 2. 공유 메모리 크기 설정 (구조체 크기만큼 물리적 할당)
	if (ftruncate(shm_fd, sizeof(ShmQueue)) == -1) 
	{
		perror("ftruncate failed");
		exit(1);
	}

	// 3. mmap 매핑 및 에러 체크
	shm_Queue = (ShmQueue *)mmap(NULL, sizeof(ShmQueue),
								PROT_READ | PROT_WRITE,
								MAP_SHARED, shm_fd, 0);

	if (shm_Queue == MAP_FAILED) 
	{
		perror("mmap failed");
		exit(1);
	}

	// 큐 인덱스 초기화 (생성 직후 한 번만 수행)
	shm_Queue->head = 0;
	shm_Queue->tail = 0;

	// 4. 세마포어 오픈
	sem_m2p   = sem_open(SEM_M2P, O_CREAT, 0666, 0);
	sem_p2m   = sem_open(SEM_P2M, O_CREAT, 0666, 0);
	sem_mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);

	// 프로세스 초기화 및 생성
	int nTask = 0;
	char *names[] = {"taskParent", "taskChild1", "taskChild2", "taskChild3"};

	nTask = 0;
	// 1. taskParent 실행 (exec 사용 가정)
	if ((p_pid = fork()) == 0)
	{
		// output 폴더에 빌드된 taskParent 실행
		execl("./output/taskParent", names[nTask], NULL);
		perror("execl taskParent failed");
		exit(1);
	}
	procs[nTask].pid = p_pid;
	strcpy(procs[nTask].name, "taskParent");
	procs[nTask].is_running = 1;

	nTask = 1;
	// 2. taskChild1 실행
	if ((c1_pid = fork()) == 0) 
	{
		execl("./output/taskChild1", "taskChild1", NULL);
		perror("execl taskChild1 failed");
		exit(1);
	}
	procs[nTask].pid = c1_pid;
	strcpy(procs[nTask].name, names[nTask]);
	procs[nTask].is_running = 1;

	nTask = 2;
	// 3. taskChild2 실행
	if ((c2_pid = fork()) == 0) 
	{
		execl("./output/taskChild2", "taskChild2", NULL);
		perror("execl taskChild2 failed");
		exit(1);
	}
	procs[nTask].pid = c2_pid;
	strcpy(procs[nTask].name, names[nTask]);
	procs[nTask].is_running = 1;

	nTask = 3;
	// 4. taskChild3 실행
	if ((c3_pid = fork()) == 0) 
	{
		execl("./output/taskChild3", "taskChild3", NULL);
		perror("execl taskChild3 failed");
		exit(1);
	}
	procs[nTask].pid = c3_pid;
	strcpy(procs[nTask].name, names[nTask]);
	procs[nTask].is_running = 1;

	prctl(PR_SET_NAME, "SKYLAB");
	print_help();

	// 송수신 각각을 담당할 스레드 생성
	if (pthread_create(&send_tid, NULL, send_thread_func, NULL) != 0) 
	{
		perror("Failed to create send thread");
		return 1;
	}

	if (pthread_create(&recv_tid, NULL, recv_thread_func, NULL) != 0) 
	{
		perror("Failed to create recv thread");
		return 1;
	}

	//Print("[SKYLAB] Multi-threaded relay started.\n");

	// 스레드가 종료될 때까지 대기
	pthread_join(send_tid, NULL);
	pthread_join(recv_tid, NULL);

	while (1) 
	{
		;;;
	}

	return 0;
}

