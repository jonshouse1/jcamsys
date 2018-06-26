// jcamsys_common.c
//


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "jcamsys.h"


void sleep_ms(int milliseconds)
{
    usleep(milliseconds * 1000);
}


#include <sched.h>
void set_realtime()
{
        struct sched_param sparam;
        sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &sparam);
}



// ***************************************************************************************************************************
//  int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
//  int tm_min;                   /* Minutes.     [0-59] */
//  int tm_hour;                  /* Hours.       [0-23] */
//  int tm_mday;                  /* Day.         [1-31] */
//  int tm_mon;                   /* Month.       [0-11] */
//  int tm_year;                  /* Year - 1900.  */
//  int tm_wday;                  /* Day of week. [0-6] */
//  int tm_yday;                  /* Days in year.[0-365] */
//  int tm_isdst;                 /* DST.         [-1/0/1]*/
//
void datetime(void *dt, int flag_short)
{
        struct tm *current;
        time_t now;

        time(&now);
        current = localtime(&now);

        if (flag_short==TRUE)
                sprintf(dt,"%i:%i:%i", current->tm_hour, current->tm_min, current->tm_sec);
        else    sprintf(dt,"%02d/%02d/%i %02d:%02d:%02d", current->tm_mday, current->tm_mon, 
			current->tm_year+1900, current->tm_hour, current->tm_min, current->tm_sec);
}




void jc_error_text(int jcerror, int addmeaning, char*txt, int textlen)
{
	char mtext[1024];
	char etext[1024];

	bzero(txt, textlen);
	mtext[0]=0;
	etext[0]=0;
	switch (jcerror)
	{
		case JC_ERR:
			sprintf(etext,"JC_ERR");	
			sprintf(mtext,"Generic error");
		break;
		case JC_ERR_BAD_MAGIC:
			sprintf(etext,"JC_ERR_BAD_MAGIC");
			sprintf(mtext,"likely trying to use a structure that has not been initialised");
		break;
		case JC_ERR_KEY_NOT_VALID:
			sprintf(etext,"JC_ERR_KEY_NOT_VALID");
			sprintf(mtext,"An encrption key is not valid");
		break;
		case JC_ERR_WRITE_FAILED:
			sprintf(etext,"JC_ERR_WRITE_FAILED");
			sprintf(mtext,"write() call failed");
		break;
		case JC_ERR_READ_FAILED:
			sprintf(etext,"JC_ERR_READ_FAILED");
			sprintf(mtext,"write() call failed");
		break;
		case JC_ERR_BUFFER_SHORT:
			sprintf(etext,"JC_ERR_BUFFER_SHORT");
			sprintf(mtext,"Operation would overrun the length of a buffer");
		break;
		case JC_ERR_NOT_FOUND:
			sprintf(etext,"JC_ERR_NOT_FOUND");
			sprintf(mtext,"Something not found, probably a file");
		break;
		case JC_ERR_TOO_SLOW:
			sprintf(etext,"JC_ERR_TOO_SLOW");
			sprintf(mtext,"Something is taking too long");
		break;
	}
	strcpy(txt,etext);
	if (addmeaning==TRUE)
	{
		strcat(txt," [");
		strcat(txt,mtext);
		strcat(txt,"]");
	}
}



// Read an entire binary file into RAM.  return bytes read or negative value for error
int filetobuffer(char *filename, char *buf, int bufsize)
{
	int s=0;
	int fh=-1;
	int r=0;
	struct stat st;
	stat(filename, &st);
	s = st.st_size;

	if (s>bufsize)										// File bigger than buffer ?
		return(JC_ERR_BUFFER_SHORT);
	fh=open(filename,O_RDONLY);
	if (fh<0)
		return(-1);
	r=read(fh,buf,s);									// Read entire file
	close(fh);
	if (r!=s)
		return(-1);
	return(s); 
}


int buffertofile(char *filename, char *buf, int bufsize, int mode)
{
	int fd=-1;
	int wb=0;

	//errno=0;
	//fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC,0644);
	fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC,mode);
	if (fd>0)
	{
		wb=write(fd,buf,bufsize);
		close(fd);
		if (wb<0)
			return(JC_ERR);
		return(wb);
	}
	return(JC_ERR);
}



// A better get time in milliseconds
unsigned long long current_timems()
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}



// Example is common online, does not work on 64 bit machine dont use
// convert millsecond back into datetime
struct tm* GetTimeAndDate(unsigned long long milliseconds)
{
    time_t seconds = (time_t)(milliseconds/1000);
    if ((unsigned long long)seconds*1000 == milliseconds)
        return localtime(&seconds);
    return NULL; // milliseconds >= 4G*1000
}





int get_clockseconds()
{
        time_t          t;
        static struct tm       *tm;

        time(&t);
        tm=localtime(&t);
        return(tm->tm_sec);
}



// Get ip from domain name
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}


// Parse command line arguments
int parse_commandlineargs(int argc,char **argv,char *findthis)
{
        int i=0;
	//int n;
        char tm[512];

        if (argc==0)
                return(FALSE);                                                          // If no command line args then dont bother

        for (i=0;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        //n=0;
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then thats the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another agument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(TRUE);
                }
        }
        return(FALSE);
}


// Which command line argument does this string occur in
// Returns -1 if not found or the argument number if the string is found
int parse_findargument(int argc,char **argv,char *findthis)
{
        int i;
	//int n;
        char tm[512];

        if (argc==0)
                return(-1);   								// If no command line args then dont bother

        for (i=0;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        //n=0;
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then thats the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another agument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(i);
                }
        }
        return(-1);
}


int file_exists(char *fname)
{
        FILE *fp=NULL;

	fp=fopen(fname,"r");
        if (fp==NULL)
		return(FALSE);

	fclose(fp);
	return(TRUE);
}



#include <dirent.h>
pid_t proc_find(const char* name) 
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                if (!strcmp(first, name)) {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }

    }
    closedir(dir);
    return -1;
}




// Used to inc a counter if c1 == T  or c1 == F
void jc_sensor_active_changed(uint16_t* ctr, int torf, char *c1)
{
	//printf("*c1=%d != torf=%d   then ctr++ =%d\n",*c1,torf,*ctr); fflush(stdout);
        if (*c1 != torf)
	{
		*c1=torf;
                *ctr = *ctr + 1;
	}
}



// set a millisecond time variable with the current time
void jc_set_time_ms(uint32_t* t)
{
	*t=current_timems();
}



void DumpHex(const void* data, size_t size) 
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}



// https://rosettacode.org/wiki/CRC-32
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
 
uint32_t rc_crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;
 
	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}
 
	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}


