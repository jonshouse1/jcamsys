// Spawn an instance of ffplay outputting to a named pipe.
// Take data from named pipe, encode into jpeg and send to server


#define PROGNAME        "jcamsysbridge_onvif_ffmpeg"
#define VERSION 	"0.2"
#define TRUE    1
#define FALSE   0

int useyuv=FALSE;
int state=0;
char dts[32];						// date time string

#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <math.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/wait.h>
#include <sched.h>

#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <asm/types.h>
#include <jpeglib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <termios.h>
#include <stropts.h>
#include <sgtty.h>

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_settings.h"
#include "jcamsys_network.h"
#include "jcamsys_images.h"

#include "id.h"

// Prototypes
void capture_init(int useyuv, int initw, int inith);
void capture_close();

#define JC_CSTATE_INITIAL		0
#define JC_CSTATE_FIND_SERVER		1
#define JC_CSTATE_CONNECT_AND_AUTH	2
#define JC_CSTATE_RUNNING		3
#define JC_CSTATE_PAUSE_THEN_INITIAL	10

int debug=FALSE;
int verbose=FALSE;
int quit=FALSE;
uint16_t framecount=0;
uint16_t pframecount=10;
int actualfps=0;
int quality=75;
int crypted=TRUE;
int docrc=TRUE;
char cmdline[2048];
int reopen=TRUE;


// At the moment back of frame rate rather than quality
// Timing related
int latencyms=0;					// current round trip time for sending and image and seeing an 'ack'



unsigned char msgbuffer[JC_MAX_MSGLEN];
struct jcamsys_header*          hdr  =(struct jcamsys_header*)&msgbuffer;

// requests
struct jcamsys_req_sens_info*   reqn =(struct jcamsys_req_sens_info*)&msgbuffer;
struct jcamsys_req_sens_active* reqa =(struct jcamsys_req_sens_active*)&msgbuffer;
struct jcamsys_req_sens_sendme* reqs =(struct jcamsys_req_sens_sendme*)&msgbuffer;
struct jcamsys_req_sens_value*  reqv =(struct jcamsys_req_sens_value*)&msgbuffer;
struct jcamsys_req_image*       reqi =(struct jcamsys_req_image*)&msgbuffer;

// messages
struct jcamsys_header*		msgh   =(struct jcamsys_header*)&msgbuffer;
struct jcamsys_ack*		msga   =(struct jcamsys_ack*)&msgbuffer;
struct jcamsys_image*		msgi   =(struct jcamsys_image*)&msgbuffer;
struct jcamsys_sens_info*	msgf   =(struct jcamsys_sens_info*)&msgbuffer;
struct jcamsys_sens_active*	msgs   =(struct jcamsys_sens_active*)&msgbuffer;
struct jcamsys_sens_value*	msgv   =(struct jcamsys_sens_value*)&msgbuffer;
struct jcamsys_dts*		msgdts =(struct jcamsys_dts*)&msgbuffer;
struct jcamsys_camerasettings*  msgcs  =(struct jcamsys_camerasettings*)&msgbuffer;

struct jcamsys_camerasettings	jcs;
struct jcamsys_image		jci[JC_MAX_IMAGES];				// two local images 
struct jcamsys_settings		stngs;
struct jcamsys_key		ky;
struct jcamsys_sensorstate	sen;
struct jcamsys_messagemask      mm;


int keylen=0;
int fdsock=-1;
int pwlen=0;
char pw[JC_MAX_PASSWORD_LEN];
char camserver_ipaddr[32];
int full_width=-1;								// safe default on everything
int full_height=-1;


int image_reqid=0;
int image_received_reqid=0;
int image_received_time=0;
int image_decoded_time=0;
int image_request_time=0;
int sendnow=FALSE;


char serverhostname[4096];
char id[JC_IDLEN];


// TODO: These should be read from the sever
int server_maxcams=0;
int update_rate_hz=0;
int update_rate_ms=400;

struct sockaddr_in server;
char camserver_ipaddr[32];
int fd_imagesocket=-1;
pid_t pidchild;
char tempfile[1024];
uint16_t reqid=0;


void socketclosed()
{
	close(fdsock);
	fdsock=-1;								// force start all over
	state=0;
printf("socketclosed()\n"); fflush(stdout);  // should not do printf in signal hander
}


void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}


int xioctl(int fh, int request, void *arg)
{
	int r;

	do 
	{
  		r = ioctl(fh, request, arg);
 	} while (-1 == r && EINTR == errno);
 	return r;
}



void update_messagemask(struct jcamsys_messagemask* mx)
{
	int i=0;

	for (i=0;i<JC_MAX_CAMS+1;i++)
		mx->frame_changes[IFULL][i]=FALSE;					// no frame changes
	mx->sensors=FALSE;								// no sensor data
	mx->dts=TRUE;									// send date time strings to us
}



void jc_message_parser(int msgtype)
{
	switch (msgtype)
	{
		case JC_REQ_SENS_ACTIVE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_REQ_SENS_ACTIVE\n",PROGNAME,getpid());
		break;

		case JC_REQ_SENS_VALUE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_REQ_VALUE\n",PROGNAME,getpid());
		break;

		case JC_REQ_IMAGE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_REQ_IMAGE\n",PROGNAME,getpid());
		break;


		case JC_MSG_IMAGE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_IMAGE\n",PROGNAME,getpid());
		break;

		case JC_MSG_SENS_ACTIVE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_ACTIVE\n",PROGNAME,getpid());
		break;


		case JC_MSG_SENS_VALUE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_VALUE\n",PROGNAME,getpid());
		break;

		case JC_MSG_DTS:
			msgdts->dts[31]=0;
			strcpy(dts,msgdts->dts);
			if (debug==TRUE)
				printf("%s(%05d): JC_MSG_DTS [%s]\n",PROGNAME,getpid(),dts);
		break;

		case JC_MSG_CAMERASETTINGS:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_CAMERASETTINGS\n",PROGNAME,getpid());
			memcpy ( &jcs, msgcs, sizeof(struct jcamsys_camerasettings));
			jc_camera_print_struct_camerasettings( &jcs );
			full_width=jcs.width;
			full_height=jcs.height;
			update_rate_ms=jcs.update_rate_ms;
			quality=jcs.quality;
			docrc=jcs.docrc;
			sendnow=TRUE;				// now we have camera settings we can send to server
			reopen=TRUE;
		break;
	}
}





void jc_events()
{
	int i=0;
	int r=0;
	int rms=0;
	int msgtype=0;
	static unsigned int last_image_request_time_ms=0;
	unsigned int time_ms=0;
	static int timer_initial_ms=0;
	static int timer_duration_ms=0;
	//uint16_t reqid=0;


	time_ms=current_timems();
	//printf("ms=%d\n",time_ms-last_image_request_time_ms);
	if ( (time_ms - last_image_request_time_ms) > 1000)
	{
		last_image_request_time_ms=time_ms;
	}

	switch (state)
	{
		case JC_CSTATE_INITIAL:
			state++;
			dts[0]=0;
			sendnow=FALSE;									// dont send images if we dont have a valid config
		break;

		case JC_CSTATE_FIND_SERVER:								// go through this state even if -host 
			if (stngs.discover_server==TRUE)
			{
				bzero(&stngs.server_ipaddr,sizeof(stngs.server_ipaddr));
				printf("%s(%05d): trying to YAFDP discover server - ",PROGNAME,getpid());
				fflush(stdout);
				r=jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port,FALSE);
				//r=jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port,TRUE);
				switch (r)
				{
					case -2:
						printf("discover service busy\n");
						fflush(stdout);
						sleep(3);						// give it some time to complete
						return;
					break;

					case -1:
						printf("yafdp error of some sort\n");
						fflush(stdout);
						sleep(3);						// give it some time to complete
						return;
					break;
				}									// here then n>=0
				if (strlen(stngs.server_ipaddr)<6)
				{
					printf("discover failed\n");
					fflush(stdout);
					timer_duration_ms= 2 * 1000;
					state=JC_CSTATE_PAUSE_THEN_INITIAL;
				}
				else    
				{
					printf("found server [%s]\n",stngs.server_ipaddr);
					state=JC_CSTATE_CONNECT_AND_AUTH;
				}
			}
			if (  (strlen(stngs.server_ipaddr)<6) & (stngs.discover_server!=TRUE) )		// no IP supplied and not discovering one
			{
				printf("%s(%05d): need to either discover or run with -host ",PROGNAME,getpid());
				fflush(stdout);
				exit(1);
			}
		break;


		case JC_CSTATE_CONNECT_AND_AUTH:
			printf("%s(%05d): connecting to server host=%s ip=%s port=%d\n",PROGNAME,getpid(),
                                stngs.server_hostname,stngs.server_ipaddr,stngs.server_tcp_port);
			fflush(stdout);
                        fdsock=jc_connect_to_server(stngs.server_ipaddr,stngs.server_tcp_port,FALSE);
			printf("****** fdsock=%d\n",fdsock); fflush(stdout);
                        if (fdsock<=0)
                        {
				fdsock=-1;
                                printf("%s(%05d): connect failed\n",PROGNAME,getpid());
                                if (stngs.persistent_connect!=TRUE)
				{
					printf("exit 1\n");
                                        exit(1);
				}
                        }
                        else										// socket is connected
                        {
                                printf("%s(%05d): connected, doing auth : ",PROGNAME,getpid());
                                i=jcam_authenticate_with_server(fdsock, (char*)&pw, pwlen, 5000);
				if (i==JC_OK)
				{
					printf("auth ok\n"); fflush(stdout);
					jc_msg_register(&ky, fdsock, id, JC_ROLE_CAMERA);
					jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);
					state=JC_CSTATE_RUNNING;
					return;
				}
				else
				{
					close(fdsock);
					fdsock=-1;
					switch(i)
					{
						case JC_ERR_TIMEOUT:    printf("timeout");	     	break;
						case JC_ERR_BAD_AUTH:   printf("bad auth");	    	break;
						case JC_ERR:		printf("early hangup");		break;
					}
					printf("\n");
					if (i==JC_ERR_BAD_AUTH)						// no point in keep trying
					{
						printf("Failed to auth, .jcpass is unique to a machine, if in doubt remove it\n");
						exit(1);
					}
					fflush(stdout);
					sleep(1);
					state=0;
				}
                        }
		break;


		case JC_CSTATE_RUNNING:									// connected and processing messages
			rms=jcam_read_message(fdsock, &ky, (unsigned char*)&msgbuffer, sizeof(msgbuffer), &msgtype, &reqid, 1);
			if (rms==JC_HAVE_DATA)
			{
				if (debug==TRUE)
					printf("got message type %d\n",msgtype); 
				jc_message_parser(msgtype);
			}

			if (rms==-1)
			{
				printf("jcam_read_message() returned -1, closing socket\n");
				fflush(stdout);
				close(fdsock);
				fdsock=-1;
				state=0;
			}	
		break;


		case JC_CSTATE_PAUSE_THEN_INITIAL:
			if (timer_initial_ms==0)							// timer not started yet ?
			{
				timer_initial_ms=current_timems();					// get current timestamp
				return;
			}
			time_ms=current_timems();							// get time now
			if ( (time_ms - timer_initial_ms) >= timer_duration_ms)				// timeout
			{
				timer_initial_ms=0;							// cancel timer
				state=0;
			}
		break;
	}
	fflush(stdout);
	usleep(100);
}





void process_image(unsigned char *data)
{
	int cbytes=0;
	static int fn=0;
	int fi=0;
	int w,h;
	char textline[2048];


		sprintf(textline,"%s CAM%d",dts,jcs.cam);
		add_text(textline,(char*)data,full_width,full_height,full_height-10,1,0,70,0,1);
		add_text(textline,(char*)data,full_width,full_height,full_height-10,1,255,255,255,0);

		jc_image_prepare(&ky,&jci[IFULL],IFULL,jcs.cam,JC_IMAGE_TYPE_JPEG,full_width,full_height,cbytes,(char*)&id,fn++);
		cbytes=encode_jpeg_tomem((unsigned char*)data, (unsigned char*)&jci[IFULL].cdata, full_width, full_height, quality);
		jci[IFULL].cbytes = cbytes;
		jci[IFULL].optional_crc32=0;
		if (cbytes>0)
		{
			if (verbose==TRUE)
			{
				printf("tcp:"); 
				fflush(stdout);
			}

			if (actualfps>1)
				fi=1000/actualfps;					// turn measured frame per second (real)
			else	fi=1000;						// into an inter-frame period
			jci->update_rate_ms=fi;
			if (sendnow==TRUE)						// if we have a valid config
				jc_msg_image(&ky, &jci[IFULL], fdsock, docrc, reqid);	// send image
			//print_struct_jcamsys_image(jci);
		}
		else
		{
			printf("jpeg ecode failed %d \n",cbytes);	
			fflush(stdout);
		}


		if (verbose==TRUE)
		{
			printf("sc:"); 
			fflush(stdout);
		}
		w=full_width;
		h=full_height;
		//scaleimage(0,(char*)&data,&w,&h,2);					// small size preview image
}




int main( int argc, char *argv[ ] )
{
	char cmstring[1024];
	int i=0;
	int l=0;
	int op_help=FALSE;
	int argnum=0;
	char st[2048];
	char filename[2048];
	char rgb[1920*1080*3];
	int fd=-1;
	int r=0;

        srand(time(NULL));
	settings_defaults(&stngs);
	//signal(SIGINT, sig_handler);
	//signal(SIGPIPE, socketclosed);
	signal(SIGPIPE, SIG_IGN);
    	bzero(&camserver_ipaddr,sizeof(camserver_ipaddr));
	bzero(&sen,sizeof(struct jcamsys_sensorstate));
	bzero(&dts,sizeof(dts));
	bzero(&id,sizeof(id));
	update_messagemask(&mm);
	strcpy(id,JCID);

	// Setup a default mode
	jc_camera_default(&jcs);

	// Extract width and height for default mode
	jcs_camera_mode_to_wh(&jcs, (uint16_t*)&full_width, (uint16_t*)&full_height);
	printf("%d x %d\n",full_width, full_height);


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
		printf(" -yuv\t\t\tUse YUV mode, uses more CPU\n");
                printf(" -p\t\t\tTCP port number, if omitted %d will be used\n",JC_DEFAULT_TCP_PORT);
		printf(" -v\t\t\tVerbose - show messages\n");
		printf(" -host <name/ip>\tDont try discovery, just connect to this server\n");
		printf(" -debug\t\t\tDebug output useful for testing\n");
		printf(" -1\t\t\tTry to connect with server once, exit if it fails\n");
		printf(" -id <arg>\t\tOverride preset ID:%s with your own\n",id);
                exit(0);
        }

	strcpy(cmstring,"-yuv");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		useyuv=TRUE;



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
		}
		else
		{
			printf("-host <host name or ipv4 address>\n");
			exit(1);
		}
	}

	strcpy(cmstring,"-id");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		//stngs.discover_server=FALSE;
		argnum=parse_findargument(argc,argv,cmstring);
		if (argnum>0)
		{
			strcpy(st,argv[argnum]);
			if (strlen(st)<1)
			{
				printf("-id <arg>\n");
				exit(1);
			}
			strcpy(id,st);
		}
		else
		{
			printf("-id <arg>\n");
			exit(1);
		}
	}


	strcpy(cmstring,"-debug");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		verbose=TRUE;
		debug=TRUE;
	}

	// cough out messages
 	strcpy(cmstring,"-v");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		verbose=TRUE;


	strcpy(cmstring,"-1");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		stngs.persistent_connect=FALSE;


	pwlen=jcam_getormake_client_obpassword((char*)&pw, sizeof(pw));
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

	// Crypt plain text password into non plain text using key
	pwlen=jcam_crypt_buf(&ky,(char*)&pw,sizeof(pw),pwlen);


	printf("%s(%05d): starting, my id:%s\n",PROGNAME,getpid(),id);
	fflush(stdout);

int sec=0;
int psec=0;


	sprintf(filename,"myfifo");	
	unlink(filename);
	if (mkfifo(filename,0666)<0)
	{
		perror("make fifo ");
		exit(1);	
	}

full_width=640;
full_height=480;
quality=75;


int bytestoread=0;
int bytesread=0;
int buflen=0;
reopen=TRUE;


//TODO: Fix, we have a large static buffer, now the child has a new one the same same.  maybe system or pexec or summink
	pid_t p=fork();
	if (p==0)
	{
		close(fd);
		sprintf(cmdline,"ffmpeg -y -i 'rtsp://10.10.10.170:554/user=admin&password=&channel=1&stream=0.sdp?' -pix_fmt rgb24 -vf scale=%dx%d -c:v rawvideo -map 0:v -f rawvideo myfifo ",full_width,full_height);
		printf("doing [%s]\n",cmdline);
		fflush(stdout);
		system(cmdline);
	}



quit=FALSE;
	while (quit!=TRUE)
	{
		if (reopen==TRUE)
		{
			printf("doing reopen\n"); fflush(stdout);
			reopen=FALSE;

			printf("opening pipe\n"); 
			fflush(stdout);
			if (fd>0)
				//for (i=0;i<1000;i++)
					//r=read(fd,&rgb,full_width*full_height*3);			// Read entire RGB frame
				close(fd);
				fd=-1;
			if (fd<0)
				fd=open(filename,O_RDONLY);
			printf("fd=%d\n",fd);
			fflush(stdout);
		}


		// Read RGB data until we have a complete frame
		buflen = full_width * full_height * 3;
		bytestoread=buflen;
		bytesread=0;

		do
		{
			r=read(fd,&rgb[bytesread],bytestoread);
			//printf("r=%d\n",r); fflush(stdout);
			if (r>0)
			{
				bytesread = bytesread + r;
				bytestoread = bytestoread -r;
			}
		} while ( (r!=0) & (bytestoread>0) );

		printf("%s(%05d): got frame length=%d\n",PROGNAME,getpid(),buflen);
		fflush(stdout);

		process_image((unsigned char*) &rgb);

		sec=get_clockseconds();
		if (sec!=psec)
		{
			psec=sec;
			actualfps=framecount;
			if (verbose!=TRUE)
			{
				if (fdsock>0)
				printf("%s(%05d): cam=%d\tDTS[%s]\tactualfps=%d\t\n",PROGNAME,getpid(),jcs.cam,dts,actualfps);
				fflush(stdout);
			}
			framecount=0;
		}

		usleep(100);
		jc_events();
 	}
	close(fd);
	return(0);
}



