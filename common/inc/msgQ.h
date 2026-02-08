/******************************************************************************
*
* msgQ.h
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
#ifndef _MESSAGE_QUEUE_
#define _MESSAGE_QUEUE_

#include <semaphore.h>
#include <pthread.h>

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
#define MSG_SIZE 				1024
#define QUEUE_SIZE 				64

#define SHM_NAME 				"/shm_queue"

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
typedef struct 
{
	int to_id;
	int from_id;
	unsigned char data[MSG_SIZE];
	int length;

} Message;

typedef struct
{
	int head; // 4bytes
	int tail; // 4bytes
	Message msg[QUEUE_SIZE];
	sem_t empty;
	sem_t full;
	sem_t mutex;

} MessageQueue;

typedef struct
{
	sem_t signal;
	sem_t mutex;
	int head; // 4bytes
	int tail; // 4bytes
	Message msg[QUEUE_SIZE];

} ShmQueue;

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
extern int Create_Queue(MessageQueue *q);
extern void Put_MsgQueue(MessageQueue *q, const Message *msg);
extern int Get_MsgQueue(MessageQueue *q, Message *msg);
extern void Close_Queue(MessageQueue *q);

extern int Create_ShmQueue(char *shm, ShmQueue **shmQ);
extern int Open_ShmQueue(char *shm, ShmQueue **shmQ);
extern int Put_ShmMsgQueue(ShmQueue *shmQ, Message *msg);
extern int Get_ShmMsgQueue(ShmQueue *shmQ, Message *msg);
extern int Close_ShmQueue(char *shm, ShmQueue *shmQ);

#endif /* _MESSAGE_QUEUE_ */
