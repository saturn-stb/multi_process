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
	q->front = 0;
	q->rear = 0;
	
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
void Put_MsgQueue(MessageQueue *q, const MessageData *msg) 
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

	memcpy(&q->buffer[q->rear], msg, sizeof(MessageData));
	q->rear = (q->rear + 1) % QUEUE_SIZE;

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
int Get_MsgQueue(MessageQueue *q, MessageData *msg) 
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
	memcpy(msg, &q->buffer[q->front], sizeof(MessageData));
	q->front = (q->front + 1) % QUEUE_SIZE;

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
SharedQueue* Create_ShmQueue(const char *name)
{
	// 1. 공유 메모리 객체 오픈 (파일처럼 생성)
	// O_CREAT: 없으면 생성, O_RDWR: 읽기/쓰기 권한
	int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	if (fd == -1) 
	{
		Print("Failed to shm_open");
		return NULL;
	}

    // 2. 공유 메모리 크기 설정 (구조체 크기만큼 확보)
    if (ftruncate(fd, sizeof(SharedQueue)) == -1)
	{
        Print("Failed to ftruncate");
        close(fd);
        return NULL;
    }

    // 3. 메모리 맵핑 (파일 기술자를 메모리 주소로 연결)
    SharedQueue *q = (SharedQueue *)mmap(NULL, sizeof(SharedQueue), 
                                        PROT_READ | PROT_WRITE, 
                                        MAP_SHARED, fd, 0);
    if (q == MAP_FAILED)
	{
        Print("Failed to mmap");
        close(fd);
        return NULL;
    }

	// [중요] 최초 생성자(O_CREAT)인 경우에만 수행
	// 기존에 남아있던 'germany' 같은 쓰레기 데이터를 완전히 제거합니다.
	memset(q, 0x0, sizeof(SharedQueue)); 

	// 3. 세마포어 초기화 (pshared = 1 필수)
	//sem_m2p   = sem_open(SEM_M2P, 0);
	//sem_p2m   = sem_open(SEM_P2M, 0);
	//sem_mutex = sem_open(SEM_MUTEX, 0);	
	if (sem_init(&q->mutex, 1, 1) == -1) { Print("sem_init mutex failed"); return NULL; }
	if (sem_init(&q->empty, 1, QUEUE_SIZE) == -1) { Print("sem_init empty failed"); return NULL; }
	if (sem_init(&q->full, 1, 0) == -1) { Print("sem_init full failed"); return NULL; }

	q->front = 0;
	q->rear = 0;
	
	return q;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Put_ShmMsgQueue(SharedQueue *q, const MessageData *msg) 
{
    // [1] empty 세마포어 대기: 빈 공간이 생길 때까지
    while (sem_wait(&q->empty) == -1)
	{
        if (errno == EINTR) 
        {
			//Print("sem_wait full %s (errno %d)", strerror(errno), errno);
			continue; // 시그널 인터럽트 시 재시도
        }
		
        Print("sem_wait empty failed");
        return -1;
    }

    // [2] mutex 획득: 큐 접근 권한 확보
    while (sem_wait(&q->mutex) == -1) 
	{
        if (errno == EINTR) continue;
        //Print("sem_wait mutex failed");
        // [중요] 이미 감소시킨 empty 카운트를 다시 복구시켜야 큐가 꼬이지 않습니다.
        if (sem_post(&q->empty) == -1) 
		{
            Print("critical sem_post recovery empty failed");
        }
        return -1;
    }
	
    // [중요] 기존 잔상 데이터를 지우고 새 데이터를 복사
    memset(&q->buffer[q->rear], 0x0, sizeof(MessageData));
	memcpy(&q->buffer[q->rear], msg, sizeof(MessageData));
	q->rear = (q->rear + 1) % QUEUE_SIZE;

	sem_post(&q->mutex);
	sem_post(&q->full);

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Get_ShmMsgQueue(SharedQueue *q, MessageData *msg)
{
    // [1] full 세마포어 대기: 데이터가 들어올 때까지
    while (sem_wait(&q->full) == -1)
	{
        if (errno == EINTR) 
        {
			//Print("sem_wait full %s (errno %d)", strerror(errno), errno);
			continue; 
        }
        Print("sem_wait full failed");
        return -1;
    }
	
    // [2] mutex 획득: 큐 접근 권한
    while (sem_wait(&q->mutex) == -1) 
	{
        if (errno == EINTR)
        {
			//Print("sem_wait full %s (errno %d)", strerror(errno), errno);
			continue;
        }
        Print("sem_wait mutex failed");
        sem_post(&q->full); // full 카운트를 다시 원복 시켜야 함
        return -1;
    }

	memcpy(msg, &q->buffer[q->front], sizeof(MessageData));
	q->front = (q->front + 1) % QUEUE_SIZE;

	sem_post(&q->mutex);
	sem_post(&q->empty);

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int Get_ShmMessage_InOrder(SharedQueue *q, MessageData *msg, int my_id) 
{
    // [1] 데이터가 있는지 먼저 확인 (기다림)
	//Print("[%s] Got full, waiting for mutex...\n", __func__);

    // [2] Mutex 획득
    while (sem_wait(&q->mutex) == -1) 
	{
        if (errno == EINTR) 
        {
			continue;
        }
        Print("sem_wait mutex failed");
        sem_post(&q->full); // 기다렸던 데이터 권한 반납
        return -1;
    }

    while (sem_wait(&q->full) == -1) 
	{
        if (errno == EINTR) // Interrupted System Call
        {
			//Print("sem_wait full %s (errno %d)", strerror(errno), errno);
			continue;
        }
        Print("sem_wait full failed");
        return -1;
    }

	//Print("[%s] Inside critical section!\n", __func__);

    // [3] 맨 앞 메시지 검사 (Peek)
    MessageData *front_msg = &q->buffer[q->front];

    if (front_msg->to_id != my_id) 
	{
        // [내 메시지가 아닌 경우]
        // 중요: 소모했던 full 카운트를 반드시 다시 돌려주어야 
        // 다른 프로세스가 이 메시지를 확인할 수 있습니다.
        sem_post(&q->full); 
        sem_post(&q->mutex);
        
        // CPU 과점유 방지를 위해 아주 짧은 휴식 권장
		//DelayUs(100);
        return 0; 
    }
	
	// [내 메시지인 경우] 실제 꺼내기 수행
	memcpy(msg, front_msg, sizeof(MessageData));
	memset(front_msg, 0, sizeof(MessageData)); // 잔상 제거
	q->front = (q->front + 1) % QUEUE_SIZE;
	
	sem_post(&q->mutex);
	sem_post(&q->empty); // 생산자에게 빈 공간 알림
	
	return 1; 
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void Close_ShmQueue(SharedQueue *q, int fd, const char *name) 
{
	// 1. 생성된 세마포어들을 파괴합니다.
	// 더 이상 세마포어를 사용하지 않음을 OS에 알리고 자원을 반환합니다.
	if (sem_destroy(&q->mutex) == -1)
	{
		Print("sem_destroy mutex failed");
	}

	if (sem_destroy(&q->empty) == -1)
	{
		Print("sem_destroy empty failed");
	}

	if (sem_destroy(&q->full) == -1) 
	{
		Print("sem_destroy full failed");
	}

    // 2. 메모리 매핑 해제 (munmap)
    if (munmap(q, sizeof(SharedQueue)) == -1) 
	{
        Print("Failed to munmap");
    }

    // 3. 파일 기술자 닫기
    if (close(fd) == -1)
	{
        Print("Failed to close shm fd");
    }

    // 4. 시스템에서 공유 메모리 객체 삭제
    // 이 함수를 호출해야 /dev/shm/ 에서 파일이 실제로 사라집니다.
    if (name != NULL) 
	{
        if (shm_unlink(name) == -1) 
		{
            Print("Failed to shm_unlink");
        }
    }

}

