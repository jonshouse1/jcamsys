// jcamsys_archiver.c
//

#define PROGNAME        "jc_archiver"
#define VERSION         "0.4"
#define UPDATE_STATS_EVERY_SECONDS 	60 * 10

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "jcamsys.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_network.h"
#include "jcamsys_sensors.h"
#include "jcamsys_modes.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_archiver.h"
#include "jcamsys_keyexchange.h"
#include "jcamsys_images.h"
#include "id.h"

extern int verbose;
extern int debug;
extern struct sharedmem* jcsm;

uint64_t	lastwritten[JC_MAX_IMAGES][JC_MAX_CAMS+1];					// the time we last wrote a file
char 		pfilepath[JC_MAX_IMAGES][8192];


void arstate(int torf)
{
	static int ptorf=-1;

	if (torf!=ptorf)
	{
		ptorf=torf;
		jcsm->ss.archiving_ok=torf;
		jcsm->se.enable_archiver=torf;
		jcsm->se_changed++;
	}
}



// Make a directory tree
// note  *filepath is distroyed by strtok
int jc_archiver_mkdirtree(char *filepath)
{
	char *pch;
	char nfp[2048];
	int r=0;

	nfp[0]=0;
	if (strlen(filepath)<2)
		return(JC_ERR);
	pch = strtok (filepath,"/");
	strcpy(nfp,jcsm->se.archive_path);
	while (pch != NULL)
	{
		strcat(nfp,"/");
		strcat(nfp,pch);
		pch = strtok (NULL, "/");

		if (jcsm->se.archive_verbose==TRUE)
			printf("mkdir %s\n",nfp);
		r=mkdir(nfp,0777);								// may be partially duplicated path so errors ok here
		if ( (r!=0) & (jcsm->se.archive_verbose==TRUE) )
			printf("mkdir %s failed\n",nfp);
	}
	return(JC_OK);
}


// Archive_path/camera/img/year/month/day/hour/min/milliseconds_since_epoc.jpg
// With a maximum frame rate of 30Hz each 'min' directory could contain 1800 files !
void jc_archiver_add_image(struct jcamsys_image* msgi)
{
	char filepath[JC_MAX_IMAGES][8192];
	char filename[8192];
	char msfilename[128];
	char st[2048];
        struct tm *itm;
        unsigned long t;
	static unsigned long int lastgoodmsg=0;
	int fd=-1;
	int r=0;

	if (strlen(jcsm->se.archive_path)<=0)							// archiver not configured, avoid crashing out
		return;
	if (jcsm->se.enable_archiver!=TRUE)
		return;
	if (msgi->img != jcsm->se.archive_image_size)
		return;										// at the moment we archive only one image size


	t=msgi->timestamp/1000;									// take the non millseconds part of the image reception time
        itm=localtime((time_t*)&t);
        //strftime(st, sizeof(st), "%Y-%m-%d %H:%M:%S", itm);
	//printf("itm=%s\n",st); fflush(stdout);


	// time to write file again ?
	if ( (current_timems() - lastwritten[msgi->img][msgi->cam] > jcsm->se.archive_rate_ms ) | (jcsm->se.archive_rate_ms==0) )
	{
		sprintf(msfilename,"JC%02dI%02d_%llu.jpg",msgi->cam,msgi->img,(long long unsigned int)jcsm->ci[msgi->img][msgi->cam].timestamp);
		sprintf(filepath[msgi->img],"imagearchive/C%02d/I%02d/Y%04d/M%02d/D%02d/H%02d/m%02d",
			msgi->cam,msgi->img,itm->tm_year+1900,itm->tm_mon+1,itm->tm_mday,itm->tm_hour,itm->tm_min);

		//printf("filename=%s  pfilename=%s\n",filepath[msgi->img],pfilepath[msgi->img]); fflush(stdout);
		if (strcmp(pfilepath[msgi->img],filepath[msgi->img])!=0)			// file path changed?
		{
			strcpy(pfilepath[msgi->img],filepath[msgi->img]);
			jc_archiver_mkdirtree(pfilepath[msgi->img]);
			strcpy(pfilepath[msgi->img],filepath[msgi->img]);
		}

		sprintf(filename,"%s/%s/%s",jcsm->se.archive_path,filepath[msgi->img],msfilename);
		if (jcsm->se.archive_verbose==TRUE)
		{
			printf("write %s %d bytes\n",filename,msgi->cbytes); 
			fflush(stdout);
		}

		// Write file to disk
		fd=open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0777);
		if (fd>0)
		{
			r=write(fd,msgi->cdata,msgi->cbytes);
			if (r==msgi->cbytes)								// good write ?
			{
				jcsm->ss.archiving_ok=TRUE;
				if (current_timems()-lastgoodmsg>3000)
				{
					sprintf(st,"Good write (%s)",filename);
					strcpy(jcsm->archive_comment,st);
					jcsm->archive_comment_changed++;
					lastgoodmsg=current_timems();
				}
			}
			else	jcsm->ss.archiving_ok=FALSE;
			close(fd);
		}
		if ( (fd<=0) | ( r < msgi->cbytes) )	// it file did not open or bytes written not image size
		{
			sprintf(st,"%s(%05d): Error [%s]  writing [%s]\n",PROGNAME,getpid(),strerror(errno),filename);
			fprintf(stderr,"%s",st);
			fflush(stderr);
			strcpy(jcsm->archive_comment,st);
			jcsm->archive_comment_changed++;
			arstate(FALSE);

			if (jcsm->se.exit_on_archive_error==TRUE)
			{
				fprintf(stderr,"%s(%05d): exit_on_archive_error is true, exiting\n",PROGNAME,getpid());
				jcsm->quit=TRUE;							// shut whole system down
			}
		}

		lastwritten[msgi->img][msgi->cam]=current_timems();					// note when we wrote it
		jcsm->st.archive_files_written++;							// update the stats	
	}
}




void ar_control()
{
	static unsigned long int lastchecktime=0;
	unsigned long int currtime=0;
	int percentfull=0;

	currtime=current_timems();
	//printf("%lu > %lu ?\n",currtime,lastchecktime); fflush(stdout);
	if (currtime-lastchecktime>60*1000)								// once a min
	{
		lastchecktime=currtime;
		percentfull=volumepercent(jcsm->se.archive_path);
		//printf("%s(%05d): Archive volume is %d%% full\n",PROGNAME,getpid(),percentfull);
		//fflush(stdout);	

		if (percentfull>90)
		{
			printf("%s(%05d): Archive volume is %d%% full\n",PROGNAME,getpid(),percentfull);
			fflush(stdout);	

			// Find the oldest hour for each camera and remove it
		}
	}
}



// update the archive file statistics.
void update_archive_stats()
{
	//jcsm->st.archive_numfiles;
	//jcsm->st.archive_oldest;

	// oldest
	// find -type f -printf '%T+ %p\n' | sort | head -n 1

	// number of files
	// find DIR_NAME -type f | wc -l

}



void jc_archiver()
{
	int i=0;
	int c=0;
	int archiver=100;
	uint32_t lastmessaget=0;
	uint32_t laststattime=0;
	char st[2048];

	fflush(stdout);
	printf("%s(%05d): started process\n",PROGNAME,getpid());

	if (mkdir (jcsm->se.archive_path,0777)==-1)
	{
		sprintf(st,"%s(%05d): Error [%s]\n",PROGNAME,getpid(),strerror(errno));
		jcsm->archive_comment_changed++;
	}

	// Stagger the starting times, with luck even out disk access a little
	for (i=0;i<JC_MAX_IMAGES;i++)									// initialise with a non zero value
	{
		for (c=0;c<JC_MAX_CAMS+1;c++)
		{
			lastwritten[i][c]=current_timems()-(jcsm->se.archive_rate_ms*c);		// fakeup a last written time
		}
		pfilepath[i][0]=0;
	}


	while (jcsm->quit!=TRUE)
	{
//jcsm->se.archiving_ok=TRUE;
		if (archiver!=jcsm->se.enable_archiver)							// flag changed state?
		{
			//if (jcsm->se.enable_archiver==TRUE)
				//sprintf(st,"%s(%05d): starting\n",PROGNAME,getpid());
			//else	sprintf(st,"%s(%05d): suspended\n",PROGNAME,getpid());
			//printf("%s",st); fflush(stdout);
			//strcpy(jcsm->archive_comment,st);
			//jcsm->archive_comment_changed++;
			archiver=jcsm->se.enable_archiver;
		}

		if ( (jcsm->se.enable_archiver==TRUE) & (strlen(jcsm->se.archive_path)==0) & ((uint32_t)current_timems()-lastmessaget > 2000) )	// archiver not configured
		{
			arstate(FALSE);			// archiver disabled
			lastmessaget=current_timems();
			sprintf(st,"%s(%05d): Archiver not configured, no archive path\n",PROGNAME,getpid());
			printf("%s",st); fflush(stdout);
			strcpy(jcsm->archive_comment,st);
			jcsm->archive_comment_changed++;
		}


		if ( (uint32_t)current_times()-laststattime > UPDATE_STATS_EVERY_SECONDS ) 
		{
			laststattime=current_times();
			update_archive_stats();
		}

		if (jcsm->se.enable_archiver==TRUE)
			ar_control();
		usleep(1000);
	}
	printf("%s(%05d): exiting\n",PROGNAME,getpid());
	exit(0);
}

