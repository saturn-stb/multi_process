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

#include "msg_def.h"

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
#define MSG_SIZE 64
#define MAX_TASK 4 // except taskMain, taskParent, taskChild1, taskChild2, taskChild3

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
struct msg_t 
{
	long id;
	long sub_id;
	char text[MSG_SIZE];

};

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
static pthread_mutex_t msg_mutex;

static pid_t p_pid, c1_pid, c2_pid, c3_pid;
static int msgid;

ProcInfo procs[MAX_TASK]; // taskParent, taskChild1, taskChild2, taskChild3

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
	printf("--- Control Commands ---\n" 
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
	printf("\n--- Process Status Report ---\n");
	for (i = 0; i < MAX_TASK; i++) 
	{
		printf("Name: %-10s | PID: %-6d | Status: %s\n", 
				procs[i].name, procs[i].pid, procs[i].is_running ? "RUNNING" : "PAUSED");
	}
	printf("-----------------------------\n");
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
		printf("[Main] Unknown target: %s\n", target);
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
				printf("[Main] %s PAUSED\n", target);
			} 
			else if (strcmp(command, "resume") == 0) 
			{
				kill(procs[i].pid, SIGCONT);
				procs[i].is_running = 1;
				printf("[Main] %s RESUMED\n", target);
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
	printf("\n[Main] Terminating all processes...\n");
	kill(p_pid, SIGKILL); 
	kill(c1_pid, SIGKILL); 
	kill(c2_pid, SIGKILL);
	kill(c3_pid, SIGKILL);
	msgctl(msgid, IPC_RMID, NULL);
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
	struct msg_t send_msg;

	(void)arg;

	printf("[Main-Send] Enter commands to send to Parent:\n");

	while (1) 
	{
		printf("Command > ");
		fflush(stdout);

		// 1. 입력 버퍼 초기화 (쓰레기 값 방지)
		memset(input, 0, MSG_SIZE);
		memset(send_msg.text, 0, MSG_SIZE);		

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

			pthread_mutex_lock(&msg_mutex);

			// 3. 일반 메시지 전송
			send_msg.id = MSG_SEND_BOSS_TO_PARENT; // Parent행 메시지 타입
			strcpy(send_msg.text, input);

			if (msgsnd(msgid, &send_msg, sizeof(send_msg.text), 0) == -1) 
			{
				perror("Main msgsnd failed");
			}

			pthread_mutex_unlock(&msg_mutex);
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
	struct msg_t revc_msg;

	(void)arg;

	while (1) 
	{
		memset(&revc_msg, 0, sizeof(struct msg_t));		
		// Type 30 (Parent가 보낸 최종 ACK)만 선별 수신
		if (msgrcv(msgid, &revc_msg, sizeof(revc_msg.text), MSG_RECV_BOSS_FROM_PARENT, IPC_NOWAIT) != -1)
		{
			pthread_mutex_lock(&msg_mutex);
			printf("\n");
			printf("\n[Main-Recv] Received from parent [ Child%ld : %s ]\n", revc_msg.sub_id, revc_msg.text);
			printf("Command > "); // UI 유지를 위한 프롬프트 재출력
			fflush(stdout);
			pthread_mutex_unlock(&msg_mutex);
		}
		else
		{
			if (errno != ENOMSG) 
			{
				perror("msgrcv error");
				break;
			}

			// 큐에 메시지가 없는 경우: CPU 점유율을 위해 아주 짧게 휴식
			//usleep(10000); // 10ms
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
	creat("progfile", 0644);

	key_t key = ftok("progfile", 65);
	msgid = msgget(key, 0666 | IPC_CREAT);
	signal(SIGINT, cleanup_and_exit);

	pthread_t send_tid, recv_tid;

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

	prctl(PR_SET_NAME, "taskMain");
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

	if (pthread_mutex_init(&msg_mutex, NULL) != 0)
	{
		perror("Mutex init failed");
		return 1;
	}

	//printf("[taskMain] Multi-threaded relay started.\n");

	// 스레드가 종료될 때까지 대기
	pthread_join(send_tid, NULL);
	pthread_join(recv_tid, NULL);

	while (1) 
	{
		;;;
	}

	return 0;
}

