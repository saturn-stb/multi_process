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

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
static pthread_mutex_t msg_mutex;

static int msgid;
static int work_done = 0;

static char _msg_text[MSG_SIZE];

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
		if (msgrcv(msgid, &recv_msg, MSG_SIZE, MSG_RECV_CHILD2_FROM_PARENT, IPC_NOWAIT) != -1)
		{
			pthread_mutex_lock(&msg_mutex);
			printf("\n");
			printf("[Child2-Recv] Received from parent : %s\n", recv_msg.text);

			memset(_msg_text, 0x0, sizeof(_msg_text));
			memcpy(_msg_text, recv_msg.text, strlen(recv_msg.text));

			work_done = 1; 
			pthread_mutex_unlock(&msg_mutex);
		}
		else
		{
			if (errno != ENOMSG)
			{
				perror("child2 msgrcv error");
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

			send_msg.id = MSG_SEND_CHILD2_TO_PARENT;
			memset(send_msg.text, 0x0, sizeof(send_msg.text));
			memcpy(send_msg.text, _msg_text, strlen(_msg_text));

			printf("[Child2-Send] Sent to Parent : %s\n", send_msg.text);
			if (msgsnd(msgid, &send_msg, MSG_SIZE, 0) == -1) 
			{
				perror("child2 msgsnd error");
			}

			pthread_mutex_unlock(&msg_mutex);
			work_done = 0; // 플래그 리셋
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
	pthread_t send_tid, recv_tid;

	prctl(PR_SET_NAME, "taskChild2");

	key_t key = ftok("progfile_child", 66);
	msgid = msgget(key, 0666 | IPC_CREAT);

	if (pthread_mutex_init(&msg_mutex, NULL) != 0) 
	{
		perror("Mutex init failed");
		return 1;
	}

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

	pthread_join(send_tid, NULL);
	pthread_join(recv_tid, NULL);
	
	while (1) 
	{
		;;;
	}

	return 0;
}

