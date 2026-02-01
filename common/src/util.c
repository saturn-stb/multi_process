/******************************************************************************
*
* util.c
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

//FILEIO
#include <sys/stat.h>
#include <dirent.h>

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
#define MAXHEXLINE 32	/* the maximum number of bytes to put in one line */
#define MAXFILESIZE 65535

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
int memory[MAXFILESIZE];		/* the memory is global */
	

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
void hexDump(char *desc, void *addr, int len) 
{
    int i = 0;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

	memset(buff, 0x0, sizeof(buff));
	
    // Output description if given.
    if (desc != NULL)
    {
        printf ("%s:len=%d\n", desc, len);
    }

	if(addr == NULL)
	{
		return;
	}

    // Process every byte in the data.
    for (i = 0; i < len; i++) 
	{
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) 
		{
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            {
                printf("  %s\n", buff);
            }

            // Output the offset.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) 
		{
            buff[i % 16] = '.';
        } 
		else 
		{
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) 
	{
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void hexDumpToFile(FILE *file, char *desc, void *addr, int len) 
{
    int i = 0;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

	memset(buff, 0x0, sizeof(buff));

    // Output description if given.
    if (desc != NULL)
    {
        fprintf (file, "%s:len=%d\n", desc, len);
    }

	if(addr == NULL)
	{
		return;
	}

    // Process every byte in the data.
    for (i = 0; i < len; i++) 
	{
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) 
		{
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            {
                fprintf (file, "  %s\n", buff);
            }

            // Output the offset.
            fprintf (file, "  %04x ", i);
        }

        // Now the hex code for the specific character.
        fprintf (file, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) 
		{
            buff[i % 16] = '.';
        } 
		else 
		{
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) 
	{
        fprintf (file, "   ");
        i++;
    }

    // And print the final ASCII bit.
    fprintf (file, "  %s\n", buff);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
unsigned int Random(unsigned int repeat)
{
	unsigned int random = 0; // ?????? ???? ????

	Print("_Random [");
	srand(time(NULL));
	for (int i = 0; i < repeat; i++) 
	{ 
		random = rand()%5; // ???? ????
		Print(" %d ", random); // ???
	}
	Print("]\n");

	return random;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int makeFolder(char *path, char *folder)
{
	DIR 		   *dir_info;
	struct dirent  *dir_entry;

	mkdir(folder, 0755); // ???? ?????? ??? ???? ????

	dir_info = opendir(path);
	if (NULL != dir_info)
	{
		while(dir_entry = readdir(dir_info))
		{ 
			// ???? ??? ??? ??? ????? ???? ???
			//Print("%s\n", dir_entry->d_name);
		}
		
		closedir(dir_info);

		return 0;
	}

	return 1;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int makeFile(char *outputFile, char *src, unsigned int length)
{
    FILE *fp = NULL;
	size_t writeSize = 0;

	/* open the file */ 
	fp = fopen(outputFile, "wb");
	if(fp != NULL)
	{
		writeSize = fwrite(src, 1, length, fp);
		if(writeSize != length)
		{
			Print("Error!!! file %s write failed!!! (%ld, %d)\n", outputFile, writeSize, length);
		}
		else
		{
			//Print("file %s write success!!! (%ld bytes)\n", outputFile, writeSize);
		}

		/* close the header file */
		fclose(fp);
	}

	return 0;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int StrICmp(const char* s1, const char* s2) 
{
  if(s1 == NULL) return 1;
  if(s2 == NULL) return 1;

#ifdef HAVE_STRCASECMP
  return strcasecmp(s1, s2);
#else
  while (tolower((unsigned char) *s1) == tolower((unsigned char) *s2)) {
    if (*s1 == '\0')
      return 0;
    s1++; s2++;
  }

  return (int) tolower((unsigned char) *s1) -
    (int) tolower((unsigned char) *s2);
#endif /* !HAVE_STRCASECMP */
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void Sleep(unsigned long count) 
{
	unsigned long n = 0;
	
	while(n++ < count);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void DelayUs(unsigned long us) 
{
	usleep();
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void DelayMs(unsigned long ms) 
{
	DelayUs(1000*ms);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
int parse_hex_line(theline, bytes, addr, num, code)
char *theline;
int *addr, *num, *code, bytes[];
{
	int sum, len, cksum;
	char *ptr;
	
	*num = 0;
	if (theline[0] != ':') return 0;
	if (strlen(theline) < 11) return 0;
	ptr = theline+1;
	if (!sscanf(ptr, "%02x", &len)) return 0;
	ptr += 2;
	if ( strlen(theline) < (11 + (len * 2)) ) return 0;
	if (!sscanf(ptr, "%04x", addr)) return 0;
	ptr += 4;
	  /* printf("Line: length=%d Addr=%d\n", len, *addr); */
	if (!sscanf(ptr, "%02x", code)) return 0;
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
	while(*num != len) {
		if (!sscanf(ptr, "%02x", &bytes[*num])) return 0;
		ptr += 2;
		sum += bytes[*num] & 255;
		(*num)++;
		if (*num >= 256) return 0;
	}
	if (!sscanf(ptr, "%02x", &cksum)) return 0;
	if ( ((sum & 255) + (cksum & 255)) & 255 ) return 0; /* checksum error */
	return 1;
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void load_file(filename)
char *filename;
{
	char line[1000];
	FILE *fin;
	int addr, n, status, bytes[256];
	int i, total=0, lineno=1;
	int minaddr=MAXFILESIZE, maxaddr=0;

	if (strlen(filename) == 0) {
		printf("   Can't load a file without the filename.");
		printf("  '?' for help\n");
		return;
	}
	fin = fopen(filename, "r");
	if (fin == NULL) {
		printf("   Can't open file '%s' for reading.\n", filename);
		return;
	}
	while (!feof(fin) && !ferror(fin)) {
		line[0] = '\0';
		fgets(line, 1000, fin);
		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
		if (line[strlen(line)-1] == '\r') line[strlen(line)-1] = '\0';
		if (parse_hex_line(line, bytes, &addr, &n, &status)) {
			if (status == 0) {  /* data */
				for(i=0; i<=(n-1); i++) {
					memory[addr] = bytes[i] & 255;
					total++;
					if (addr < minaddr) minaddr = addr;
					if (addr > maxaddr) maxaddr = addr;
					addr++;
				}
			}
			if (status == 1) {  /* end of file */
				fclose(fin);
				printf("   Loaded %d bytes between:", total);
				printf(" %08X to %08X\n", minaddr, maxaddr);
				return;
			}
			if (status == 2) ;  /* begin of file */
		} else {
			printf("   Error: '%s', line: %d\n", filename, lineno);
		}
		lineno++;
	}
}


/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void save_file(command)
char *command;
{
	int begin, end, addr;
	char *ptr, filename[200];
	FILE *fhex;

	ptr = command+1;
	while (isspace(*ptr)) ptr++;
	if (*ptr == '\0') {
		printf("   Must specify address range and filename\n");
		return;
		}
	if (sscanf(ptr, "%x%x%s", &begin, &end, filename) < 3) {
		printf("   Invalid addresses or filename,\n");
		printf("    usage: S begin_addr end_addr filename\n");
		printf("    the addresses must be hexidecimal format\n");
		return;
	}
	begin &= 65535;
	end &= 65535;
	if (begin > end) {
		printf("   Begin address must be less than end address.\n");
		return;
	}
	fhex = fopen(filename, "w");
	if (fhex == NULL) {
		printf("   Can't open '%s' for writing.\n", filename);
		return;
	}
	for (addr=begin; addr <= end; addr++)
		hexout(fhex, memory[addr], addr, 0);
	hexout(fhex, 0, 0, 1);
	printf("Memory %04X to %04X written to '%s'\n", begin, end, filename);
}

/*-----------------------------------------------------------------------------
*
*
*
*---------------------------------------------------------------------------*/
void hexout(fhex, byte, memory_location, end)
FILE *fhex;  /* the file to put intel hex into */
int byte, memory_location, end;
{
	static int byte_buffer[MAXHEXLINE];
	static int last_mem, buffer_pos, buffer_addr;
	static int writing_in_progress=0;
	register int i, sum;

	if (!writing_in_progress) {
		/* initial condition setup */
		last_mem = memory_location-1;
		buffer_pos = 0;
		buffer_addr = memory_location;
		writing_in_progress = 1;
		}

	if ( (memory_location != (last_mem+1)) || (buffer_pos >= MAXHEXLINE) \
	 || ((end) && (buffer_pos > 0)) ) {
		/* it's time to dump the buffer to a line in the file */
		fprintf(fhex, ":%02X%04X00", buffer_pos, buffer_addr);
		sum = buffer_pos + ((buffer_addr>>8)&255) + (buffer_addr&255);
		for (i=0; i < buffer_pos; i++) {
			fprintf(fhex, "%02X", byte_buffer[i]&255);
			sum += byte_buffer[i]&255;
		}
		fprintf(fhex, "%02X\n", (-sum)&255);
		buffer_addr = memory_location;
		buffer_pos = 0;
	}

	if (end) {
		fprintf(fhex, ":00000001FF\n");  /* end of file marker */
		fclose(fhex);
		writing_in_progress = 0;
	}
		
	last_mem = memory_location;
	byte_buffer[buffer_pos] = byte & 255;
	buffer_pos++;
}

