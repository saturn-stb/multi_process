/******************************************************************************
*
* msg_def.h
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
#ifndef _MSG_DEF_
#define _MSG_DEF_

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

/* BOSS */
// taskMain <==> taskParent
#define MSG_SEND_BOSS_TO_PARENT 		1
#define MSG_RECV_BOSS_FROM_PARENT 		30
// taskParent <==> taskMain
#define MSG_SEND_PARENT_TO_BOSS 		30
#define MSG_RECV_PARENT_FROM_BOSS 		1


/* PARENT */
// taskParent <==> taskChild1
#define MSG_SEND_PARENT_TO_CHILD1 		2
#define MSG_RECV_PARENT_FROM_CHILD1		10
// taskChild1 <==> taskParent
#define MSG_SEND_CHILD1_TO_PARENT 		10
#define MSG_RECV_CHILD1_FROM_PARENT 	2

// taskParent <==> taskChild2
#define MSG_SEND_PARENT_TO_CHILD2 		3
#define MSG_RECV_PARENT_FROM_CHILD2 	20
// taskChild2 <==> taskParent
#define MSG_SEND_CHILD2_TO_PARENT 		20
#define MSG_RECV_CHILD2_FROM_PARENT 	3


/* CHILD */
// taskChild1 <==> taskChild2
#define MSG_SEND_CHILD1_TO_CHILD2 		100
#define MSG_RECV_CHILD1_FROM_CHILD2 	200
// taskChild2 <==> taskChild1
#define MSG_SEND_CHILD2_TO_CHILD1 		200
#define MSG_RECV_CHILD2_FROM_CHILD1 	100


// taskChild1 <==> taskChild3
#define MSG_SEND_CHILD1_TO_CHILD3 		101
#define MSG_RECV_CHILD1_FROM_CHILD3 	201
// taskChild3 <==> taskChild1
#define MSG_SEND_CHILD3_TO_CHILD1 		201
#define MSG_RECV_CHILD3_FROM_CHILD1 	101


// taskChild2 <==> taskChild3
#define MSG_SEND_CHILD2_TO_CHILD3 		102
#define MSG_RECV_CHILD2_FROM_CHILD3 	202
// taskChild3 <==> taskChild2
#define MSG_SEND_CHILD3_TO_CHILD2 		202
#define MSG_RECV_CHILD3_FROM_CHILD2 	102

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

#endif
