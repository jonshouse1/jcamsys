// jcamsysclient.c

#define PROGNAME        "jcamsysclient"
#define VERSION         "0.4"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>


#include "jcamsys.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_protocol.h"
#include "jcamsys_settings.h"
#include "jcamsys_network.h"

unsigned char msgbuffer[JC_MAX_MSGLEN];

struct jcamsys_settings		stngs;
struct jcamsys_key		ky;
struct jcamsys_sensorstate	sen;
int keylen=0;

// Server should never send us reqeusts
void jc_callback_req(int reqtype,int reqid,int senstype,int sensor,int porf) {}


// Server sent an ACK for a request we made, reqid indentifies the request
void jc_callback_gotack(int reqid)
{
}

void jc_callback_image(int reqid,int cam, unsigned char*data, int datalen, int image_type, int width, int height, int crypted)
{
}

void jc_callback_sensor_value(int reqid, int senstype, int sensor, float fvalue, int ivalue,char*cvalue)
{
}


void process_loop(int fdsock)
{
	int r=0;
printf("main process loop,auth went ok\n"); fflush(stdout);

	r=jcam_read_and_process_message(fdsock, &ky, &sen, (unsigned char*)&msgbuffer, sizeof(msgbuffer), TRUE);
}



int main(int argc, char **argv)
{
	unsigned char sbuf1[16384];
	unsigned char sbuf2[16384];
	char pw[JC_MAX_PASSWORD_LEN];
	int argnum;
        char cmstring[1024];
        int op_help=FALSE;
	char st[2048];
	int i=0;
	int l=0;
	int fdsock=-1;
	int quit=FALSE;
	int pwlen=0;

        srand(time(NULL));
	settings_defaults(&stngs);

        // Parse command line options and set flags first
        strcpy(cmstring,"-h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"--h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"-help");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"--help");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        if (op_help==TRUE)
        {
                printf("%s <Options>\n",PROGNAME);
                printf(" -p\t\t\tTCP port number, if omitted %d will be used\n",JC_DEFAULT_TCP_PORT);
                printf(" -noenc\t\t\tDisable encryption, not recommended\n");
                printf(" -D\t\t\tRun as Daemon\n");
		printf(" -host <name/ip>\tDont try discovery, just connect to this server\n");
		printf(" -1\t\t\tTry to connect with server once, exit if it fails\n");
		printf(" -cam <cam>\t\tThe camera we want\n");
		printf(" -preview\t\tFetch the smaller thumbnail image\n");
		printf(" -fps <num>\t\tKeep fething images at this rate\n");
		printf(" -o <filename>\t\tFilename for image\n");
                exit(0);
        }

	i=JC_DEFAULT_TCP_PORT;
	strcpy(cmstring,"-p");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		argnum=parse_findargument(argc,argv,cmstring);
		if (argnum>0)
		{
			strcpy(st,argv[argnum]);
			if (strlen(st)<1)
			{
				printf("-p <value>\n");
				exit(1);
			}
			else    i=atoi(st);
		}
	}
	if (i>0)
		stngs.server_tcp_port=i;

	strcpy(cmstring,"-host");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		argnum=parse_findargument(argc,argv,cmstring);
		if (argnum>0)
		{
			strcpy(st,argv[argnum]);
			if (strlen(st)<1)
			{
				printf("-host <host name or ipv4 address>\n");
				exit(1);
			}
			strcpy(stngs.server_hostname,st);
			stngs.discover_server=FALSE;						// user supplied ip, dont try and override that
			if (jc_hostname_to_ip((char*)&stngs.server_hostname,(char*)&stngs.server_ipaddr)<0)
			{
				printf("Error looking up ip for host %s\n",st);
				exit(1);
			}
		}
		else
		{
			printf("-host <host name or ipv4 address>\n");
			exit(1);
		}
	}
	strcpy(cmstring,"-1");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		stngs.persistent_connect=FALSE;


	pwlen=jcam_getormake_client_obpassword((unsigned char*)&pw, sizeof(pw));
//printf("pwlen=%d pw=[%s]\n",pwlen,pw); fflush(stdout);
	if (pwlen<JC_MIN_PASSWORD_LEN)
	{
		printf("Bad password, only %d long, must be at least %d\n",l,JC_MIN_PASSWORD_LEN);
		exit(1);
	}

	printf("%s(%05d): %s version %s (c)2018 J.Andrews (jon@jonshouse.co.uk)\n",PROGNAME,getpid(),PROGNAME,VERSION);
	keylen=jcam_read_keyfile(stngs.filename_site_key,&ky);
	if (keylen<0)
	{
		printf("Site key [%s] missing or bad, use jcmakekey\n",stngs.filename_site_key);
		exit(1);
	}
	else	printf("%s(%05d): got valid key, length=%d\n",PROGNAME,getpid(),keylen);
	fflush(stdout);


	while (quit!=TRUE)
	{
		if (stngs.discover_server==TRUE)
		{
			printf("%s(%05d): trying to YAFDP discover server - ",PROGNAME,getpid());
			fflush(stdout);
			jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port);	
			if (strlen(stngs.server_ipaddr)<6)
				printf("discover failed\n");
			else	printf("found server\n");
		}

		if (strlen(stngs.server_ipaddr)>=6)
		{
			printf("%s(%05d): connecting to server host=%s ip=%s port=%d\n",PROGNAME,getpid(),
				stngs.server_hostname,stngs.server_ipaddr,stngs.server_tcp_port);
			fdsock=-1;
			fdsock=jc_connect_to_server(stngs.server_ipaddr,stngs.server_tcp_port,TRUE);
			if (fdsock<0)
			{
				printf("%s(%05d): connect failed\n",PROGNAME,getpid());
				if (stngs.persistent_connect!=TRUE)
					exit(1);
			}
			else										// Connected OK
			{
//int jcam_crypt_buf(struct jcamsys_key* key,unsigned char *data, int datasize, int datalen)
				pw[pwlen++]='\n';
				i=jcam_crypt_buf(&ky,(unsigned char*)&pw,sizeof(pw),pwlen);
				i=jcam_authenticate_with_server(fdsock, (char*)&pw, pwlen, 9000);
				if (i!=JC_OK)
				{
					close(fdsock);
					switch(i)
					{
						case JC_ERR_TIMEOUT:	printf("timeout ");	break;
						case JC_ERR_BAD_AUTH:	printf("bad auth ");	break;
					}
					printf("error, failed to connect with server\n");
					exit(1);
				}
				else	
				{
					process_loop(fdsock);
					close(fdsock);
				}
			}
		}
		fflush(stdout);	
		if ( (stngs.persistent_connect!=TRUE) & (fdsock<0) )
			exit(1);
		sleep(2);
	}
	if (fdsock>0)
		close(fdsock);
}

