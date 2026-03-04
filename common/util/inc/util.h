/******************************************************************************
*
* util.h
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
#ifndef _UTIL_
#define _UTIL_

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
#define UNUSED (void *)0
#define ASSERT(expression)		do						\
								{						\
									if(expression == 0)	\
									{					\
										while(1);		\
									}					\
								}while(0)
							

#define E_Complete   0x0000
#define E_Failed     0x0001

#define TRUE         1
#define FALSE        (!TRUE)

#if 1
/*
	31: 빨간색 (에러 메시지용)
	32: 초록색 (성공/정상 로그용)
	33: 노란색 (경고 메시지용)
	34: 파란색 (정보 메시지용)
	35: 마젠타 (보라색)
*/
#define DLOG(fmt, ...) \
	fprintf(stderr, "\033[0;34m[%s:%d]\033[0m " fmt, \
	__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DLOG(fmt, ...) ((void)0)
#endif

#if 1
#define Print(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define Print(fmt, ...) ((void)0)
#endif

#if 1
#define CPrint(fmt, ...) \
	fprintf(stderr, "\033[0;32m\033[0m " fmt, ##__VA_ARGS__)
#else
#define Print(fmt, ...) ((void)0)
#endif


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
void hexDump(char *desc, void *addr, int len);
void hexDumpToFile(FILE *file, char *desc, void *addr, int len);
unsigned int Random(unsigned int repeat);
int makeFolder(char *path, char *folder);
int makeFile(char *outputFile, char *src, unsigned int length);
int StrICmp(const char* s1, const char* s2);
void Sleep(unsigned long count);
void DelayUs(unsigned long us);
void DelayMs(unsigned long us);

void load_file(char *filename);

/* this writes a part of memory[] to an intel hex file */
void save_file(char *command);

/* this is used by load_file to get each line of intex hex */
int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);

/* this does the dirty work of writing an intel hex file */
/* caution, static buffering is used, so it is necessary */
/* to call it with end=1 when finsihed to flush the buffer */
/* and close the file */
void hexout(FILE *fhex, int byte, int memory_location, int end);

#endif /* _UTIL_ */
