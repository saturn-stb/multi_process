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
static ShmQueue *shm_RecvQ = NULL;
static ShmQueue *shm_SendQ = NULL;

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
	Message msg;

	(void)arg;

	while (1) 
	{
		memset(&msg, 0x0, sizeof(Message));

		msg.to_id = PROC_ID_CHILD1;
		if(Get_ShmMsgQueue(shm_RecvQ, &msg) == 0)
		{
			Print("\n[CHILD1-RECV] Message from PARENT : %s\n", msg.data);
			memcpy(&_msg_text[0], msg.data, MSG_SIZE);
			work_done = 1; 
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
	Message msg;

	(void)arg;

	while (1) 
	{
		// work_done이 1이 될 때까지 대기 (Busy-wait 방지를 위한 usleep)
		if (!work_done) 
		{
			DelayUs(10000); // 10ms
			continue;
		}

		memset(&msg, 0x0, sizeof(Message));

		msg.to_id = PROC_ID_CHILD3;
		msg.from_id = PROC_ID_CHILD2;
		memcpy(msg.data, &_msg_text[0], MSG_SIZE);
		Print("\n[CHILD3-SEND] message to CHILD3 : %s\n", msg.data);
		Put_ShmMsgQueue(shm_SendQ, &msg);

		work_done = 0;
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

	prctl(PR_SET_NAME, "taskChild1");

	if (Create_ShmQueue(SHM_RECV_NAME, &shm_RecvQ) != 0)
	{
		Print("shm_open failed\n");
		exit(1);
	}

	if (Create_ShmQueue(SHM_SEND_NAME, &shm_SendQ) != 0)
	{
		Print("shm_open failed\n");
		exit(1);
	}

	if (pthread_create(&send_tid, NULL, send_thread_func, NULL) != 0) 
	{
		Print("Failed to create send thread\n");
		return 1;
	}

	if (pthread_create(&recv_tid, NULL, recv_thread_func, NULL) != 0) 
	{
		Print("Failed to create recv thread\n");
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

