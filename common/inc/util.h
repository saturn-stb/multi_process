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
							

#define Print printf

#define E_Complete   0x0000
#define E_Failed     0x0001

#define TRUE         1
#define FALSE        (!TRUE)

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
