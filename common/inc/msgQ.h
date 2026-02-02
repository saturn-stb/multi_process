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
#define MSG_SIZE 		256
#define QUEUE_SIZE 		8

#define SHM_NAME 	"/mp_shm"

#define SEM_M2P   	"/sem_m2p"
#define SEM_P2M   	"/sem_p2m"
#define SEM_P2C   	"/sem_p2c"
#define SEM_C2P   	"/sem_c2p"
#define SEM_MUTEX 	"/sem_mutex"

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
// 메시지 개별 데이터를 담는 구조체
typedef struct 
{
	int to_id;
	int from_id;
	unsigned char payload[MSG_SIZE];
	int length; // 실제 담긴 데이터의 크기를 기록하면 더 유용합니다.

} MessageData;

typedef struct 
{
	MessageData buffer[QUEUE_SIZE]; // MessageData 구조체 배열로 변경
	int front;
	int rear;
	sem_t mutex; // 큐 접근 보호 (이진 세마포어)
	sem_t empty; // 빈 슬롯 개수 (초기값: QUEUE_SIZE)
	sem_t full;  // 채워진 슬롯 개수 (초기값: 0)

} MessageQueue;

typedef struct 
{
	MessageData buffer[QUEUE_SIZE]; // MessageData 구조체 배열로 변경
	int front;
	int rear;
	sem_t mutex;
	sem_t empty;
	sem_t full;

} SharedQueue;

typedef struct 
{
	int proc; // 4bytes
	int from_proc; // 4bytes
	unsigned char data[MSG_SIZE]; // 48bytes

} Job;

typedef struct
{
	int head; // 4bytes
	int tail; // 4bytes
	Job jobs[QUEUE_SIZE]; // 52bytes

} ShmQueue; // 64bytes

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
extern void Put_MsgQueue(MessageQueue *q, const MessageData *msg);
extern int Get_MsgQueue(MessageQueue *q, MessageData *msg);
extern void Close_Queue(MessageQueue *q);

extern SharedQueue* Create_ShmQueue(const char *name);
extern int Put_ShmMsgQueue(SharedQueue *q, const MessageData *msg);
extern int Get_ShmMsgQueue(SharedQueue *q, MessageData *msg);
extern int Get_ShmMessage_InOrder(SharedQueue *q, MessageData *msg, int my_id);
extern void Close_ShmQueue(SharedQueue *q, int fd, const char *name);

#endif /* _MESSAGE_QUEUE_ */
