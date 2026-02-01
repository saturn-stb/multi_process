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
static int shm_fd = -1;
static ShmQueue *shm_Queue = NULL;
static sem_t *sem_m2p = NULL; // Main to Parent
static sem_t *sem_p2m = NULL; // Parent to Main
static sem_t *sem_mutex = NULL;

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
	unsigned char msg_data[MSG_SIZE];

	(void)arg;

	while (1) 
	{
		memset(msg_data, 0, MSG_SIZE);

        // 1. 부모로부터의 신호 대기 (이게 먼저 와야 함)
        sem_wait(sem_m2p);
        // 2. 임계 구역 진입
        sem_wait(sem_mutex);
		
        if (shm_Queue != NULL) 
        {
            // 3. 큐가 비어있는지 확인
            if (shm_Queue->head == shm_Queue->tail) 
            {
				Print("\n[CHILD1-RECV] Queue Full! Dropping.\n");
                sem_post(sem_mutex);
                continue;
            }

			// 4. My 메시지 인지 확인
            int proc_id = shm_Queue->jobs[shm_Queue->tail].proc;
            int from_proc_id = shm_Queue->jobs[shm_Queue->tail].from_proc;
			if((proc_id != PROC_ID_CHILD1) || (from_proc_id != PROC_ID_PARENT))
			{
				Print("\n[CHILD1-RECV] ID mismatch!!! (0x%02x)\n", proc_id);
				
				// 내 메시지가 아닌 경우: 락을 풀고 세마포어를 다시 올려서 다른 프로세스가 보게 함
	            sem_post(sem_mutex);
				sem_post(sem_m2p);
				
				// CPU 점유율 과다 방지를 위한 미세 대기 (Spin-lock 방지)
				usleep(100);
	            continue;
			}
			
			// 5. 데이터 읽기 (Consumer: tail 사용)
			int pos = shm_Queue->tail;
			shm_Queue->tail = (pos + 1) % QUEUE_SIZE;
			
	        memcpy(msg_data, shm_Queue->jobs[pos].data, MSG_SIZE);
			Print("\n[CHILD1-RECV] Message from PARENT : %s\n", msg_data);
			
			sem_post(sem_mutex);
			
			// 6. 수신 데이터 저장 및 처리
			memcpy(&_msg_text[0], msg_data, MSG_SIZE);

			work_done = 1; 
        }
        else 
        {
            sem_post(sem_mutex);
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
	unsigned char msg_data[MSG_SIZE];

	(void)arg;

	while (1)
	{
		memset(msg_data, 0x0, sizeof(msg_data));
		
        // work_done이 1이 될 때까지 대기 (Busy-wait 방지를 위한 usleep)
        if (!work_done) 
        {
            usleep(10000); // 10ms
            continue;
        }

        sem_wait(sem_mutex);
        if (shm_Queue != NULL) 
        {
	        // Queue Full 체크
	        if (((shm_Queue->head + 1) % QUEUE_SIZE) == shm_Queue->tail) 
	        {
	            Print("\n[CHILD1-SEND] Queue Full, waiting...\n");
	            sem_post(sem_mutex);
	            usleep(50000);
	            continue;
	        }

	        // 데이터 복사 (처리 결과 전송)
			memcpy(msg_data, _msg_text, MSG_SIZE);

#if 1
			int pos = shm_Queue->head;
			shm_Queue->jobs[pos].proc = PROC_ID_CHILD2;
			shm_Queue->jobs[pos].from_proc = PROC_ID_CHILD1;
			memcpy(shm_Queue->jobs[pos].data, msg_data, MSG_SIZE);
			shm_Queue->head = (pos + 1) % QUEUE_SIZE;

			Print("\n[CHILD1-SEND] Message to CHILD2 : %s\n", msg_data);
#else
            int pos = shm_Queue->head;
			shm_Queue->jobs[pos].proc = PROC_ID_PARENT;
			shm_Queue->jobs[pos].from_proc = PROC_ID_CHILD1;
	        memcpy(shm_Queue->jobs[pos].data, msg_data, MSG_SIZE);
	        shm_Queue->head = (pos + 1) % QUEUE_SIZE;
	        
	        Print("\n[CHILD1-SEND] Message to PARENT : %s\n", msg_data);
#endif
	        work_done = 0; // 플래그 리셋
	        sem_post(sem_mutex);
	        
	        // taskMain의 수신 스레드에게 신호 전송
	        sem_post(sem_p2m); 
        }
		else
		{
            sem_post(sem_mutex);
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
	pthread_t send_tid, recv_tid;

	prctl(PR_SET_NAME, "taskChild1");

	// 1. 공유 메모리 오픈 (O_CREAT 추가하여 없으면 생성)
	shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
	if (shm_fd == -1)
	{
		perror("shm_open failed");
		exit(1);
	}

#if 1
	// 2. 공유 메모리 크기 설정 (구조체 크기만큼 물리적 할당)
	if (ftruncate(shm_fd, sizeof(ShmQueue)) == -1) 
	{
		perror("ftruncate failed");
		exit(1);
	}
#endif

	// 3. mmap 매핑 및 에러 체크
	shm_Queue = (ShmQueue *)mmap(NULL, sizeof(ShmQueue),
								PROT_READ | PROT_WRITE,
								MAP_SHARED, shm_fd, 0);

	if (shm_Queue == MAP_FAILED) 
	{
		perror("mmap failed");
		exit(1);
	}

	// 세마포어 연결
	sem_m2p   = sem_open(SEM_M2P, 0);
	sem_p2m   = sem_open(SEM_P2M, 0);
	sem_mutex = sem_open(SEM_MUTEX, 0);

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

