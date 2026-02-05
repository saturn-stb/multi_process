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
#include <termios.h>
#include <limits.h>
#include <libgen.h> // dirname 사용 시

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
#define MAX_TASK 	4 // except SKYLAB, taskParent, taskChild1, taskChild2, taskChild3
#define MAX_PATH 	256 // except SKYLAB, taskParent, taskChild1, taskChild2, taskChild3

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
typedef enum
{
	PROC_ST_PAUSE = 0,
	PROC_ST_RUNNING,
	PROC_ST_KILL,
	PROC_ST_SEG_FAULT,
	PROC_ST_EXIT, // generally exit
	MAX_PROC_ST

}ProcStatus;

// 프로세스 정보 구조체
typedef struct 
{
	pid_t pid;
	char name[20];
	char path[128]; // 실행 파일 경로 저장 (예: "./output/taskChild1")
	int is_running; // 1: Running, 0: Paused, 2:Kill, 3:Segmentation Fault

} ProcInfo;

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static ShmQueue *shm_Q = NULL;

static ProcInfo procs[MAX_TASK]; // taskParent, taskChild1, taskChild2, taskChild3
static char my_path[MAX_PATH];
static char* root_dir = NULL;

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
			"1. pause / resue / kill / exec  [taskParent/taskChild1/taskChild2/taskChild3]\n"
			"2. status\n"
			"3. mode   [1, using getch]\n"
			"4. help\n"
			"5. exit\n"
			"6. [Any text for IPC]\n------------------------\n");
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void print_status(void) 
{
	int i = 0;
	Print("\n--------------- Process Status Report ---------------\n");
	for (i = 0; i < MAX_TASK; i++) 
	{
		Print("Name: %-10s | PID: %-6d | Status: %s\n", 
				procs[i].name, procs[i].pid, 
				(procs[i].is_running == PROC_ST_RUNNING) ? "RUNNING" : 
				(procs[i].is_running == PROC_ST_KILL) ? "KILL" : 
				(procs[i].is_running == PROC_ST_SEG_FAULT) ? "SEG.FAULT" : "PAUSED");
	}
	Print("-------------------------------------------------------\n");
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void respawn_process(int index) 
{
    pid_t new_pid = fork();
    if (new_pid == 0) 
	{
        // 미리 저장해둔 절대 경로로 즉시 실행
        execl(procs[index].path, procs[index].name, (char *)NULL);
        exit(1);
    }
	else if (new_pid > 0)
	{
        procs[index].pid = new_pid;
        procs[index].is_running = 1;
    }
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void control_process(const char* command, const char* target) 
{
	int i = 0;
	
	for (i = 0; i < MAX_TASK; i++) 
	{
		if (strcmp(procs[i].name, target) == 0) 
		{
			if (strcmp(command, "pause") == 0) 
			{
				kill(procs[i].pid, SIGSTOP);
				Print("[Main] %s PAUSED\n", target);
			} 
			else if (strcmp(command, "resume") == 0) 
			{
				kill(procs[i].pid, SIGCONT);
				Print("[Main] %s RESUMED\n", target);
			}
			else if (strcmp(command, "kill") == 0) 
			{
				kill(procs[i].pid, SIGKILL);
				Print("[Main] %s KILL\n", target);
			} 
			else if (strcmp(command, "segfault") == 0) 
			{
				kill(procs[i].pid, SIGSEGV);
				Print("[Main] %s Segmentation fault\n", target);
			} 
			else if (strcmp(command, "exec") == 0) 
			{
				if(procs[i].is_running == PROC_ST_KILL)
				{
					respawn_process(i);
					Print("[Main] %s EXEC\n", target);
					break;
				}
			} 
		}
	}
}

/*-----------------------------------------------------------------------------
* 자식 프로세스의 상태를 감시하는 핸들러
*
*
*---------------------------------------------------------------------------*/
void sigchld_handler(int sig) 
{
	int status;
	pid_t pid;
	int i = 0;

	// WUNTRACED: 정지(STOP) 감지
	// WCONTINUED: 재개(CONT) 감지
	// WNOHANG: 비동기 처리
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) 
	{
		for (i = 0; i < MAX_TASK; i++) 
		{
			if (procs[i].pid == pid) 
			{
				// 1. 정상 종료된 경우 (exit 호출 등)
				if (WIFEXITED(status)) 
				{
					Print("\n[ALARM] %s (PID: %d) exited with status %d\n", 
						  procs[i].name, pid, WEXITSTATUS(status));
					procs[i].is_running = PROC_ST_SEG_FAULT;
				}
				// 2. 시그널에 의해 강제 종료된 경우 (kill -9, Segfault 등)
				else if (WIFSIGNALED(status)) 
				{
					Print("\n[ALARM] %s (PID: %d) terminated by signal %d (%s)\n", 
						  procs[i].name, pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
					procs[i].is_running = PROC_ST_KILL;
				}
				// 3. SIGSTOP에 의해 정지된 경우
				else if (WIFSTOPPED(status)) 
				{
					Print("\n[ALARM] %s (PID: %d) stopped by signal %d\n", 
						  procs[i].name, pid, WSTOPSIG(status));
					procs[i].is_running = PROC_ST_PAUSE;
				}
				// 4. SIGCONT에 의해 재개된 경우
				else if (WIFCONTINUED(status)) 
				{
					Print("\n[ALARM] %s (PID: %d) resumed\n", procs[i].name, pid);
					procs[i].is_running = PROC_ST_SEG_FAULT;
				}
				break;
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
	int i;
	(void)sig;
	Print("\n[Main] Terminating all processes...\n");
	for (i = 0; i < MAX_TASK; i++) 
	{
		kill(procs[i].pid, SIGKILL); 
	}
	exit(0);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int getch(void) 
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // ICANON: 엔터 없이 입력받기
    // ECHO: 입력한 키 화면에 표시 안 하기
    newt.c_lflag &= ~(ICANON | ECHO);

    // IXON: Ctrl+S, Ctrl+Q 흐름 제어 비활성화 (매우 중요!)
    newt.c_iflag &= ~(IXON | ICRNL); 

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void* send_thread_func(void* arg)
{
	char input[MSG_SIZE];
    int mode = 0; // 0: fgets 모드, 1: getch 모드

	(void)arg;

	while (1) 
	{
		if(mode == 1)
		{
            Print("\n[Mode 1: getch] Press any key (Ctrl+Q to return Mode 0): \n");

			while(mode)
			{
	            fflush(stdout);
				
	            int ch = getch();

	            // Ctrl + Q 체크 (ASCII 17)
	            if (ch == 17)
				{
	                Print("\nSwitching back to fgets mode...\n");
	                mode = 0;
					continue;
	            }
				
				Print("Input Key: %c (Code: %d, 0x%02X)\n", ch, ch, ch);
			}
        }
		
		Print("Command > ");
		fflush(stdout);

		// 1. 입력 버퍼 초기화 (쓰레기 값 방지)
		memset(input, 0, MSG_SIZE);

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

			// "mode 1" 입력 시 전환
			if (strcmp(input, "mode 1") == 0) 
			{
				Print("Switching to getch mode (Ctrl+Q to exit)...\n");
				mode = 1;
				continue;
			}

			// 2. pause/resume 제어
			char cmd[10], target[20];
			if (sscanf(input, "%s %s", cmd, target) == 2)
			{
				if (strcmp(cmd, "pause") == 0 || strcmp(cmd, "resume") == 0
					 || strcmp(cmd, "kill") == 0  || strcmp(cmd, "segfault") == 0
					  || strcmp(cmd, "exec") == 0) 
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

			static Message msg;
			memset(&msg, 0x0, sizeof(Message));
			
			msg.to_id = PROC_ID_PARENT;
			msg.from_id = PROC_ID_SKYLAB;
			memcpy(msg.data, input, strlen(input));
			msg.length = strlen(input);
			Print("\n[SKYLAB-SEND] message to PARENT : %s\n", msg.data);
			Put_ShmMsgQueue(shm_Q, &msg);
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
	Message msg;

	(void)arg;

	while (1) 
	{
		memset(&msg, 0x0, sizeof(Message));

		msg.to_id = PROC_ID_SKYLAB;
		if(Get_ShmMsgQueue(shm_Q, &msg) == 0)
		{
			Print("\n[SKYLAB-RECV] Message from PARENT : %s\n", msg.data);
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
    struct sigaction sa;

	memset(my_path, 0x0, sizeof(my_path));
	
	// 1. 현재 SKYLAB의 실행 절대 경로 획득
	ssize_t len = readlink("/proc/self/exe", my_path, sizeof(my_path) - 1);	
	if (len != -1) 
	{
		my_path[len] = '\0'; // dirname은 원본을 수정할 수 있으므로 주의해서 사용
		root_dir = dirname(my_path);	 
	} 
	else
	{
		Print("readlink failed\n");
		return 1;
	}

    // SIGCHLD 핸들러는 자식 프로세스들이 생성되기 "전"에 핸들러 설정
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
	// 중요: SA_NOCLDSTOP이 없어야 SIGSTOP을 감지할 수 있습니다.
    //sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // SA_RESTART: 인터럽트된 시스템 콜 재시작
    sa.sa_flags = SA_RESTART; // SA_RESTART: 인터럽트된 시스템 콜 재시작
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
        Print("sigaction failed\n");
        return 1;
    }

	pthread_t send_tid, recv_tid;

	if (Create_ShmQueue(SHM_NAME, &shm_Q) != 0)
	{
		Print("shm_open failed\n");
		exit(1);
	}

	// 프로세스 초기화 및 생성
	int nTask = 0;
    // 2. 자식 프로세스 정보 초기화 (경로 설정)
    char *names[] = {"taskParent", "taskChild1", "taskChild2", "taskChild3"};
    for (int i = 0; i < MAX_TASK; i++) 
	{
        strcpy(procs[i].name, names[i]);
        // root_dir(현재폴더) / output / 파일명 순으로 경로 합성
        snprintf(procs[i].path, sizeof(procs[i].path), "%s/%s", root_dir, names[i]);
    }

    // 3. 프로세스 실행 (nTask 사용 예시)
    for (nTask = 0; nTask < MAX_TASK; nTask++) 
	{
        if ((procs[nTask].pid = fork()) == 0)
		{
            // 미리 계산된 절대 경로(procs[nTask].path)를 사용
            Print("Executing: %s\n", procs[nTask].path);
            
            // execl(실행경로, argv[0], ..., NULL)
            execl(procs[nTask].path, procs[nTask].name, (char *)NULL);

            // execl 성공 시 아래는 실행되지 않음
            Print("execl failed\n");
            exit(1);
        }
        procs[nTask].is_running = PROC_ST_RUNNING;
    }

	prctl(PR_SET_NAME, "SKYLAB");
	print_help();

	// 송수신 각각을 담당할 스레드 생성
	if (pthread_create(&send_tid, NULL, send_thread_func, (void *)shm_Q) != 0) 
	{
		Print("Failed to create send thread\n");
		return 1;
	}
	//pthread_detach(send_tid);

	if (pthread_create(&recv_tid, NULL, recv_thread_func, (void *)shm_Q) != 0) 
	{
		Print("Failed to create recv thread\n");
		return 1;
	}
	//pthread_detach(recv_tid);

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

