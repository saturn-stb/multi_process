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
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#include <time.h>
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

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static pthread_mutex_t msg_mutex;
static pthread_mutex_t child_msg_mutex;

static int msgid;
static int work_done = 0;

static int child_msgid;
static int child_work_done = 0;
static int child_recv_msg_done = 0x0; // 0x0:not received all message, 0x1:received messsage by child1, 0x2:received messsage by child2 

static char _msg_text[3][MSG_SIZE]; // 0:received from taskMain(boss), 1:received from taskChild1, 2:received from taskChild2

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
static void reverse_string(char* str) 
{
	if (str == NULL) return;

	int len = strlen(str);
	for (int i = 0; i < len / 2; i++)
	{
		// 양 끝의 문자를 서로 바꿈 (Swap)
		char temp = str[i];
		str[i] = str[len - 1 - i];
		str[len - 1 - i] = temp;
	}
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static void* recv_thread_func(void* arg)
{
	struct msg_t recv_msg;

	(void)arg;

	while (1) 
	{
		memset(&recv_msg, 0x0, sizeof(struct msg_t));
		if (msgrcv(msgid, &recv_msg, MSG_SIZE, MSG_RECV_PARENT_FROM_BOSS, IPC_NOWAIT) != -1)
		{
			pthread_mutex_lock(&msg_mutex);

			printf("\n");
			printf("\n");
			printf("[Parent-Recv] Received from Boss : %s\n", recv_msg.text);

			memset(&_msg_text[0], 0x0, MSG_SIZE);
			memcpy(&_msg_text[0], recv_msg.text, strlen(recv_msg.text));

			//work_done = 1;
			child_work_done = 1;
			pthread_mutex_unlock(&msg_mutex);
		}
		else
		{
			if (errno != ENOMSG) 
			{
				perror("parent msgrcv error");
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
static void* send_thread_func(void* arg)
{
	struct msg_t send_msg;

	(void)arg;

	while (1) 
	{
		if (work_done) 
		{
			pthread_mutex_lock(&msg_mutex);

			send_msg.id = MSG_SEND_PARENT_TO_BOSS;
			memset(send_msg.text, 0x0, sizeof(send_msg.text));
			if(child_recv_msg_done & 0x1)
			{
				send_msg.sub_id = 0x1;
				memcpy(send_msg.text, &_msg_text[1], MSG_SIZE);
				child_recv_msg_done &= ~(0x1);
				printf("[Parent-Send] Sent to Boss : child1 message [ %s ]\n", send_msg.text);

				if (msgsnd(msgid, &send_msg, MSG_SIZE, 0) == -1)
				{
					perror("parent msgsnd error");
				}
			}

			send_msg.id = MSG_SEND_PARENT_TO_BOSS;
			memset(send_msg.text, 0x0, sizeof(send_msg.text));
			if(child_recv_msg_done & 0x2)
			{
				send_msg.sub_id = 0x2;
				memcpy(send_msg.text, &_msg_text[2], MSG_SIZE);
				child_recv_msg_done &= ~(0x2);
				printf("[Parent-Send] Sent to Boss : child2 message [ %s ]\n", send_msg.text);

				if (msgsnd(msgid, &send_msg, MSG_SIZE, 0) == -1) 
				{
					perror("parent msgsnd error");
				}
			}

			work_done = 0; // 플래그 리셋
			pthread_mutex_unlock(&msg_mutex);
		}

		//usleep(100000); // 0.1초 간격 폴링
	}

	return NULL;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static void* child_recv_thread_func(void* arg)
{
	struct msg_t recv_msg;

	(void)arg;

	while (1) 
	{
		// 1. 자식들(Type 10, 20)로부터 ACK 수신
		// -20을 인자로 주어 10번과 20번 타입을 모두 수집합니다.
		memset(&recv_msg, 0x0, sizeof(struct msg_t));
		if (msgrcv(child_msgid, &recv_msg, MSG_SIZE, MSG_RECV_PARENT_FROM_CHILD1, IPC_NOWAIT) != -1)
		{
			pthread_mutex_lock(&msg_mutex);
			printf("\n");
			printf("[Parent-Recv] Received from child%ld : %s\n", recv_msg.id/10, recv_msg.text);

			memset(&_msg_text[1], 0x0, MSG_SIZE);
			memcpy(&_msg_text[1], recv_msg.text, MSG_SIZE);

			work_done = 1;
			//child_work_done = 1; 
			child_recv_msg_done |= 0x1;
			pthread_mutex_unlock(&msg_mutex);
		}
		else
		{
			if (errno != ENOMSG) 
			{
				perror("parent(child1) msgrcv error");
				break;
			}

			// 큐에 메시지가 없는 경우: CPU 점유율을 위해 아주 짧게 휴식
			//usleep(10000); // 10ms
		}

		if (msgrcv(child_msgid, &recv_msg, MSG_SIZE, MSG_RECV_PARENT_FROM_CHILD2, IPC_NOWAIT) != -1)
		{
			pthread_mutex_lock(&msg_mutex);
			printf("\n");
			printf("[Parent-Recv] Received from child%ld : %s\n", recv_msg.id/10, recv_msg.text);

			memset(&_msg_text[2], 0x0, MSG_SIZE);
			memcpy(&_msg_text[2], recv_msg.text, MSG_SIZE);

			work_done = 1;
			//child_work_done = 1; 
			child_recv_msg_done |= 0x2;
			pthread_mutex_unlock(&msg_mutex);
		}
		else
		{
			if (errno != ENOMSG) 
			{
				perror("parent(child2) msgrcv error");
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
static void* child_send_thread_func(void* arg)
{
	struct msg_t send_msg;

	(void)arg;

	while (1)
	{
		if (child_work_done)
		{
			pthread_mutex_lock(&msg_mutex);

			send_msg.id = MSG_SEND_PARENT_TO_CHILD1;
			memset(send_msg.text, 0x0, sizeof(send_msg.text));
			memcpy(send_msg.text, &_msg_text[0], MSG_SIZE);

			printf("[Parent-Send] Sent to Child1 : %s\n", send_msg.text);
			if (msgsnd(child_msgid, &send_msg, MSG_SIZE, 0) == -1)
			{
				perror("parent(child1) msgsnd error");
			}

			send_msg.id = MSG_SEND_PARENT_TO_CHILD2;
			memset(send_msg.text, 0x0, sizeof(send_msg.text));
			memcpy(send_msg.text, &_msg_text[0], MSG_SIZE);
			reverse_string(send_msg.text);

			printf("[Parent-Send] Sent to Child2 : %s\n", send_msg.text);
			if (msgsnd(child_msgid, &send_msg, MSG_SIZE, 0) == -1) 
			{
				perror("parent(child2) msgsnd error");
			}

			pthread_mutex_unlock(&msg_mutex);
			child_work_done = 0; // 플래그 리셋
		}

		//usleep(100000); // 0.1초 간격 폴링
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
	creat("progfile_child", 0644);

	prctl(PR_SET_NAME, "taskParent");

	key_t key = ftok("progfile", 65);
	msgid = msgget(key, 0666 | IPC_CREAT);

	key_t child_key = ftok("progfile_child", 66);
	child_msgid = msgget(child_key, 0666 | IPC_CREAT);

	if (key == -1 || child_key == -1)
	{
		perror("ftok failed - check if files exist");
		exit(1);
	}

	if (pthread_mutex_init(&msg_mutex, NULL) != 0) 
	{
		perror("Mutex init failed");
		return 1;
	}

	if (pthread_mutex_init(&child_msg_mutex, NULL) != 0)
	{
		perror("Mutex init failed");
		return 1;
	}

	pthread_t send_tid, recv_tid;
	pthread_t send_ch_tid, recv_ch_tid;

	// 송수신 각각을 담당할 스레드 생성
	if (pthread_create(&send_tid, NULL, send_thread_func, NULL) != 0) 
	{
		perror("Failed to create downstream send thread");
		return 1;
	}

	if (pthread_create(&recv_tid, NULL, recv_thread_func, NULL) != 0)
	{
		perror("Failed to create downstream recv thread");
		return 1;
	}

	// 송수신 각각을 담당할 스레드 생성
	if (pthread_create(&send_ch_tid, NULL, child_send_thread_func, NULL) != 0)
	{
		perror("Failed to create upstream send thread");
		return 1;
	}

	if (pthread_create(&recv_ch_tid, NULL, child_recv_thread_func, NULL) != 0)
	{
		perror("Failed to create upstream recv thread");
		return 1;
	}

	//printf("[taskParent] Multi-threaded relay started.\n");

	pthread_join(send_tid, NULL);
	pthread_join(recv_tid, NULL);

	pthread_join(send_ch_tid, NULL);
	pthread_join(recv_ch_tid, NULL);

	while (1) 
	{
		;;;
	}

	return 0;
}

