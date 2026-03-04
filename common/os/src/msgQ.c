/******************************************************************************
*
* msgQ.c
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
#include <ctype.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>           // shm_open, shm_unlink
#include <sys/stat.h>           /* For mode constants */
#include <fcntl.h>              /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>

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
int Create_Queue(MessageQueue *q) 
{
	q->head = 0;
	q->tail = 0;
	
	// 리턴값이 -1이면 초기화 실패
	if (sem_init(&q->mutex, 0, 1) == -1) 
	{
		Print("Failed to initialize mutex semaphore");
		return -1;
	}
	
	if (sem_init(&q->empty, 0, QUEUE_SIZE) == -1)
	{
		Print("Failed to initialize empty semaphore");
		sem_destroy(&q->mutex); // 이미 생성된 건 정리
		return -1;
	}
	
	if (sem_init(&q->full, 0, 0) == -1)
	{
		Print("Failed to initialize full semaphore");
		sem_destroy(&q->mutex);
		sem_destroy(&q->empty);
		return -1;
	}

	return 0; // 성공
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void Put_MsgQueue(MessageQueue *q, const Message *msg) 
{
    // EINTR 발생 시 다시 wait 하도록 루프 처리 (선택 사항)
	while (sem_wait(&q->empty) == -1)
	{
		if (errno == EINTR) continue; 
		Print("sem_wait failed for empty");
		return;
	}

	if (sem_wait(&q->mutex) == -1) 
	{
		Print("sem_wait failed for mutex");
		sem_post(&q->empty); // 획득했던 empty 자원 반환
		return;
	}

	memcpy(&q->msg[q->tail], msg, sizeof(Message));
	q->tail = (q->tail + 1) % QUEUE_SIZE;

	if (sem_post(&q->mutex) == -1) // 큐 접근 해제 (Unlock)
	{
		Print("sem_post failed for mutex");
	}

	if (sem_post(&q->full) == -1) // 채워진 슬롯 개수 증가 (소비자에게 알림)
	{
		Print("sem_post failed for full");
	}
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Get_MsgQueue(MessageQueue *q, Message *msg) 
{
	// 1. Full 세마포어 대기 (데이터가 있을 때까지)
	if (sem_wait(&q->full) == -1)
	{
		Print("sem_wait failed (full)");
		return -1; // 에러 발생 시 -1 반환
	}

	// 2. Mutex 획득 (임계 영역 진입)
	if (sem_wait(&q->mutex) == -1) 
	{
		Print("sem_wait failed (mutex)");
		// 이미 소모한 full 카운트를 복구시켜야 함
		sem_post(&q->full); 
		return -1;
	}

	// 데이터 추출 로직
	memcpy(msg, &q->msg[q->head], sizeof(Message));
	q->head = (q->head + 1) % QUEUE_SIZE;

	// 3. Mutex 해제
	if (sem_post(&q->mutex) == -1) 
	{
		Print("sem_post failed (mutex)");
	}

	// 4. Empty 세마포어 증가 (빈 공간 알림)
	if (sem_post(&q->empty) == -1)
	{
		Print("sem_post failed (empty)");
	}

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void Close_Queue(MessageQueue *q) 
{
	// 1. 생성된 세마포어들을 파괴합니다.
	// 더 이상 세마포어를 사용하지 않음을 OS에 알리고 자원을 반환합니다.
	if (sem_destroy(&q->mutex) == -1)
	{
		Print("mutex 파괴 실패");
	}

	if (sem_destroy(&q->empty) == -1)
	{
		Print("empty 세마포어 파괴 실패");
	}

	if (sem_destroy(&q->full) == -1) 
	{
		Print("full 세마포어 파괴 실패");
	}

	// (참고) 만약 MessageQueue 구조체를 malloc으로 생성했다면 
	// 여기서 free(q); 를 추가로 호출해야 합니다.
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Create_ShmQueue(char *shm, ShmQueue **shmQ)
{
	int shm_fd;
	ShmQueue *temp_ptr; // 임시 포인터 선언

	shm_fd = shm_open(shm, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (shm_fd == -1)
	{
		perror("shm_open failed");
		return -1;
	}

	if(ftruncate(shm_fd, sizeof(ShmQueue)) == -1)
	{
		perror("ftruncate failed");
		return -1;
	}

	// mmap 결과를 임시 포인터에 할당
	temp_ptr = (ShmQueue *)mmap(NULL, sizeof(ShmQueue),
								PROT_READ | PROT_WRITE,
								MAP_SHARED, shm_fd, 0);

	if (temp_ptr == MAP_FAILED)
	{
		perror("mmap failed");
		return -1;
	}

	// 초기화 (임시 포인터를 사용하므로 -> 연산자가 안전함)
	temp_ptr->head = 0;
	temp_ptr->tail = 0;

	// 4. 세마포어 초기화 (주소 전달 시 & 사용)
	if (sem_init(&(temp_ptr->mutex), 1, 1) == -1)
	{
		perror("sem_init mutex failed");
		return -1;
	}
	
	if (sem_init(&(temp_ptr->signal), 1, 0) == -1) 
	{
		perror("sem_init signal failed");
		return -1;
	}

	// 마지막에 외부에서 넘겨준 포인터 변수에 주소값 복사
	*shmQ = temp_ptr;

	close(shm_fd); // 매핑 후 fd는 닫아도 유지됨

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Open_ShmQueue(char *shm, ShmQueue **shmQ)
{
	int shm_fd;
	ShmQueue *temp_ptr; // 임시 포인터 선언

	shm_fd = shm_open(shm, O_RDWR, 0666);
	if (shm_fd == -1)
	{
		perror("shm_open failed");
		return -1;
	}

	// mmap 결과를 임시 포인터에 할당
	temp_ptr = (ShmQueue *)mmap(NULL, sizeof(ShmQueue),
								PROT_READ | PROT_WRITE,
								MAP_SHARED, shm_fd, 0);

	if (temp_ptr == MAP_FAILED)
	{
		perror("mmap failed");
		return -1;
	}

	// 마지막에 외부에서 넘겨준 포인터 변수에 주소값 복사
	*shmQ = temp_ptr;

	close(shm_fd); // 매핑 후 fd는 닫아도 유지됨

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Put_ShmMsgQueue(ShmQueue *shmQ, Message *msg) 
{
	if (shmQ == NULL || shmQ == MAP_FAILED) return -1;
		
	sem_wait(&(shmQ->mutex)); 
	
	// Queue Full 체크
	if (((shmQ->tail + 1) % QUEUE_SIZE) == shmQ->head) 
	{
		perror("Queue Full, waiting...");
		sem_post(&(shmQ->mutex));
		//DelayUs(50000); // 50ms
		return -1;
	}

	memcpy(&shmQ->msg[shmQ->tail], msg, sizeof(Message));
	shmQ->tail = (shmQ->tail + 1) % QUEUE_SIZE;
	
	sem_post(&(shmQ->mutex));

	// taskMain의 수신 스레드에게 신호 전송
	sem_post(&(shmQ->signal)); 

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Get_ShmMsgQueue(ShmQueue *shmQ, Message *msg)
{
	if (shmQ == NULL || shmQ == MAP_FAILED) return -1;
	
	// 신호 대기
	sem_wait(&(shmQ->signal));
	sem_wait(&(shmQ->mutex));
	
	// 큐가 비어있는지 확인
	if (shmQ->head == shmQ->tail) 
	{
		perror("Queue Full! Dropping.");
		sem_post(&(shmQ->mutex));
		return -1;
	}

	// My 메시지 인지 확인
	if(shmQ->msg[shmQ->head].to_id == msg->to_id)
	{
		// 데이터 읽기 (Consumer: head 사용)
		memcpy(msg, &(shmQ->msg[shmQ->head]), sizeof(Message));
		shmQ->head = (shmQ->head + 1) % QUEUE_SIZE;
		
		sem_post(&(shmQ->mutex));
	}
	else
	{
		// 내 메시지가 아닌 경우: 락을 풀고 세마포어를 다시 올려서 다른 프로세스가 보게 함
		sem_post(&(shmQ->mutex));
		sem_post(&(shmQ->signal));
		
		// CPU 점유율 과다 방지를 위한 미세 대기 (Spin-lock 방지)
		DelayUs(100); // 100us
		return -1;
	}
	
	return 0;
}


/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Close_ShmQueue(char *shm, ShmQueue *shmQ)
{
    if (shmQ == NULL || shmQ == MAP_FAILED) return -1;

    // 1. 세마포어 파괴 (sem_init으로 초기화한 경우)
    // 포인터가 아니므로 SEM_FAILED 체크가 필요 없으며, &를 붙여 호출합니다.
    //sem_destroy(&(shmQ->mutex));
    //sem_destroy(&(shmQ->signal));

    // 2. mmap 해제
    if (munmap(shmQ, sizeof(ShmQueue)) == -1) 
    {
        perror("munmap failed");
        // 실패하더라도 unlink는 진행하는 것이 보통입니다.
    }

    // 3. 공유 메모리 객체 이름 삭제 (시스템에서 제거)
    // 이 작업은 모든 프로세스가 종료될 때 한 번만 수행하는 것이 좋습니다.
    if (shm != NULL) 
	{
        shm_unlink(shm);
    }

    return 0;
}

