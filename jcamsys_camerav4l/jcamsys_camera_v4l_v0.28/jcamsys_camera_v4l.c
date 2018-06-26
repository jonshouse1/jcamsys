// jcamsys_camera_v4l
// Takes image data from remote cameras, archives images.
//
// on rpi modprobe bcm2835-v4l2
// list camera modes    v4l2-ctl --list-formats-ext
// for the rpi cpu core temp can be read with: cat /sys/class/thermal/thermal_zone0/temp   /1000=C
// for RPI use "disable_camera_led=1" in /boot/config.txt

// Run a wire from module to Pi GPIO, top right is GPIO21
//
//		http://www.retiisi.org.uk/v4l2/tmp/media_api/control.html
//
//		https://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html
//


#define PROGNAME        "jcamsyscamera4l"
#define VERSION 	"0.28"

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
#include <linux/videodev2.h>
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

#include <linux/videodev2.h>
#define CLEAR(x) memset(&(x), 0, sizeof(x))

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_client_settings.h"
#include "jcamsys_network.h"
#include "jcamsys_images.h"


// Prototypes
void capture_init(int useyuv, int initw, int inith);
void capture_close();

#define JC_CSTATE_INITIAL		0
#define JC_CSTATE_FIND_SERVER		1
#define JC_CSTATE_CONNECT_AND_AUTH	2
#define JC_CSTATE_RUNNING		4
#define JC_CSTATE_PAUSE_THEN_INITIAL	10

int useyuv=FALSE;
int state=0;
char dts[32];						// date time string
int rpi=FALSE;						// running on pi board?
int reinit=TRUE;
int debug=FALSE;
int verbose=FALSE;
int quit=FALSE;
uint16_t framecount=0;
uint16_t pframecount=10;
int actualfps=0;
int crypted=TRUE;
int v4lfc;						// frame counter
int v4lfps;						// measured frames per second
int prequested_width=0;
int prequested_height=0;

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
struct jcamsys_register*	msgri  =(struct jcamsys_register*)&msgbuffer;

struct jcamsys_camerasettings	jcs;
struct jcamsys_image		jci[JC_MAX_IMAGES];				// two local images 
struct jcamsys_client_settings	stngs;
struct jcamsys_key		ky;
struct jcamsys_sensorstate	sen;
struct jcamsys_messagemask      mm;
struct jcamsys_register		registration;


int keylen=0;
int fdsock=-1;
int pwlen=0;
char pw[JC_MAX_PASSWORD_LEN];
char camserver_ipaddr[32];


int image_reqid=0;
int image_received_reqid=0;
int image_received_time=0;
int image_decoded_time=0;
int image_request_time=0;
int sendnow=FALSE;


char serverhostname[4096];
//unsigned char image_raw [ 1920 * 1080 * 4];
unsigned char image_raw [ 3840 * 2160 * 4];
//char id[JC_IDLEN];


// TODO: These should be read from the sever
int server_maxcams=0;
int update_rate_hz=0;
int update_rate_ms=400;

struct sockaddr_in server;
char camserver_ipaddr[32];
int fd_imagesocket=-1;
pid_t pidchild;
char tempfile[1024];
int reqid=0;


struct v4l2_format fmt;		

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

struct buffer {
	void   *start;
	size_t  length;
};

static char            *dev_name;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;



void socketclosed()
{
	close(fdsock);
	fdsock=-1;								// force start all over
	state=0;
	//printf("socketclosed()\n"); fflush(stdout);  // should not do printf in signal hander
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
	{
		mx->frame_changes[IFULL][i]=FALSE;					// no frame changes
		mx->camera_settings[i]=FALSE;						// 
	}
	mx->sensors=FALSE;								// no sensor data
	mx->dts=TRUE;									// send date time strings to us
	mx->server_settings=FALSE;
	mx->comment=FALSE;
}



int rpi_read_soc_temp()
{
	int fd=-1;
	int r=0;
	int t=0;
	char text[1024];

	fd=open("/sys/class/thermal/thermal_zone0/temp",O_RDONLY);
	r=read(fd,&text,sizeof(text));
	if (r>0)
	{
		t=atoi(text);
		t=t/1000;
		close(fd);
		return(t);
	}
	return(-999);
}



void send_camerasettings()
{
	jcs.width=fmt.fmt.pix.width;
	jcs.height=fmt.fmt.pix.height;
	jcs.yuv=useyuv;
	jcs.width=fmt.fmt.pix.width;				// send server the actual wxh V4l gave us
	jcs.height=fmt.fmt.pix.height;

	printf("send_camerasettings()\n");
	print_struct_camerasettings( &jcs );			// what we finally settled on
	jc_msg_camerasettings(&ky, &jcs, fdsock, 0);
}




void jc_message_parser(int msgtype)
{
	int r=0;
	char cmdline[1024];
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


		case JC_MSG_SENS_VALUES:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_VALUES\n",PROGNAME,getpid());
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
			memcpy ( &jcs, msgcs, sizeof(struct jcamsys_camerasettings));			// save a local copy
			print_struct_camerasettings( &jcs );
			update_rate_ms=jcs.update_rate_ms;
			strcpy(dts,jcs.dts);								// to get us started
			if (sendnow!=TRUE)		// just coming online from an offline state
				send_camerasettings();	
			sendnow=TRUE;				// now we have camera settings we can send images to server

			// Toggle IRcut filter on/off.   1=daylight
			if (rpi==TRUE)
			{
				sprintf(cmdline,"echo %d >/sys/class/gpio/gpio21/value",jcs.ircut);
				system(cmdline);
			}
		break;

		// upon startup we load a registration record from file, if not then we register with a blank ID
		// and let the server send the registration back with an ID filled in. Store it locally
		case JC_MSG_REGISTER:
			memcpy(&registration, msgri, sizeof(struct jcamsys_register));
			printf("%s(%05d): Got registration from server, ID=%s\n",PROGNAME,getpid(),msgri->id);
			printf("%s(%05d): writing [%s]\n",PROGNAME,getpid(),stngs.filename_client_registration);
			r=savefile(stngs.filename_client_registration,(char*)&registration,sizeof(struct jcamsys_register),0777);
			if (r<=0)
			{
				printf("Error writing registration file [%s] \n",stngs.filename_client_registration);
				printf("process needs to be sudo for the first run\n");
				fflush(stdout);
				exit(1);
			}
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
	uint16_t reqid=0;
	uint64_t sendtimems;


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
			if (stngs.discover_server!=TRUE)						// -host used
				state=JC_CSTATE_CONNECT_AND_AUTH;					// skip over find_server

		break;

		case JC_CSTATE_FIND_SERVER:
			if (stngs.discover_server==TRUE)
			{
				bzero(&stngs.server_ipaddr,sizeof(stngs.server_ipaddr));
				printf("%s(%05d): trying to YAFDP discover server - ",PROGNAME,getpid());
				fflush(stdout);

				r=jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port,FALSE,TRUE);
				//printf("r=%d\n",r); fflush(stdout);  
				switch (r)
				{
					case -2:
						printf("discover service busy\n");
						fflush(stdout);
						return;
					break;

					case -1:
						printf("yafdp error of some sort\n");
						fflush(stdout);
						return;
					break;
				}									// here then n>=0
				if (strlen(stngs.server_ipaddr)<6)
				{
					printf("discover failed\n");
					fflush(stdout);
					timer_duration_ms= 2 * 1000;
					state=JC_CSTATE_PAUSE_THEN_INITIAL;
					sleep(3);
				}
				else    
				{
					printf("found server [%s] port=%d\n",stngs.server_ipaddr,stngs.server_tcp_port);
					state=JC_CSTATE_CONNECT_AND_AUTH;
					return;
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
			//printf("****** fdsock=%d\n",fdsock); fflush(stdout);
                        if (fdsock<=0)
                        {
				fdsock=-1;
                                printf("%s(%05d): connect failed\n",PROGNAME,getpid());
				sleep(1);
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
					registration.role=JC_ROLE_CAMERA;
					jc_msg_register(&ky, &registration, fdsock);			// tell server id&role
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


		case JC_CSTATE_RUNNING:									// connected and processing messages
			rms=jcam_read_message(fdsock, &ky, (unsigned char*)&msgbuffer, sizeof(msgbuffer), &msgtype, &reqid, &sendtimems, 1);
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

	}
	fflush(stdout);
	usleep(200);
}






// The capture code calls this at the frame rate, pick a frame out at our own update_rate.
// compress the image from the camera and send it to the server
// the incoming image may be any old width/height, we need to cope
// *p RGB24 bit
// isyuv == TRUE if YUV, else RGB24

// frame_number  = frame of VIDEO
// fn = frame number of image sent
// framecount = running total for frames per second calculation
void v4l_process_image(int isyuv,int iw, int ih,unsigned char *p, int size, uint16_t frame_number)
{
	char textline[1024];
	int w=0;
	int h=0;
	//unsigned int curtime=0;
	//static int prevtime=0;
	static int lastsendtime=0;
	static int delay=0;
	int fi=0;										// frame interval (ms)
	int expectedfi=0;
	static int fn=0;
	int i=0;
	int smallest=0;
int isclose=0;
static int damp=0;

	int expectedfps=1000/update_rate_ms;
	if (actualfps>=1)
		fi=1000/actualfps;
	else	fi=1000;

	expectedfi=1000/expectedfps;
	

//printf("fi=%d expectedfi=%d expectedfps=%d\tactualfps=%d delay=%d\n",fi, expectedfi, expectedfps, actualfps, delay);
	if (delay>0)
		sleep_ms(delay);

	isclose=abs(actualfps-expectedfps);
//printf("isclose=%d\n",isclose);
	if (actualfps!=expectedfps)
	{
		if (isclose>=1)
		{
			damp++;
			if (damp>10)
			{
				for (i=1;i<=isclose;i++)
				{
				if (fi<expectedfi)
					delay++;
				if (fi>expectedfi)
					delay--;
				if (delay<0)
					delay=0;
				}
				damp=0;
			}
		}
	}

//JA

		fn++;
		if (verbose==TRUE)
			printf("%s(%05d): process image(%dx%d):",PROGNAME,getpid(),iw,ih);
		if (isyuv==TRUE)
		{
			if (verbose==TRUE)
			{
				printf("YUV:"); 
				fflush(stdout);
			}
			YUV422toRGB888(iw, ih, p , (unsigned char*)&image_raw);
		}
		else
		{
			if (size!=iw*ih*3)
			{
				printf("%s(%05d): Error, expected %d, got %d!, switching to YUV\n",PROGNAME,getpid(),iw*ih*3,size);
				fflush(stdout);
				useyuv=TRUE;
				reinit=TRUE;
			}
			else	memcpy(&image_raw,p,iw*ih*3);
		}
		if (verbose==TRUE)
		{
			printf("at:"); 
			fflush(stdout);
		}


		// scale and send preview (smaller) images
		smallest=0;
		for (i=1;i<JC_MAX_IMAGES;i++)						// all possible scaled images
		{
			if (jcs.scaledown[i]==TRUE)
				smallest=i;
		}

		if (smallest<1)
			smallest=1;							// at least try the full size one

		w=iw;
		h=ih;

		if (jcs.flipv==TRUE)
			imageflipv((char*)&image_raw,w,h);
		if (jcs.fliph==TRUE)
			imagefliph((char*)&image_raw,w,h);
		framecount++;
		for (i=0;i<=smallest;i++)
		{
			textline[0]=0;
			if ( i<=2 )							// add caption text to some image sizes
			{
				if (jcs.dodatetime==TRUE)
					sprintf(textline,"%s ",dts);
				if (jcs.docaption == TRUE)
					strcat(textline,jcs.captext);
				if (strlen(textline)>0)
				{
					add_text(textline,(char*)&image_raw,w,h,h-10,1,0,70,0,1);
					add_text(textline,(char*)&image_raw,w,h,h-10,1,255,255,255,0);
				}
			}

			jci[i].cbytes=encode_jpeg_tomem((unsigned char*)&image_raw, (unsigned char*)&jci[i].cdata, w, h, jcs.quality);
			strcpy(jci[i].sid, registration.id);
			jci[i].image_type=JC_IMAGE_TYPE_JPEG;
			jci[i].quality=jcs.quality;
			jci[i].frame_number=fn;
			jci[i].update_rate_ms=fi;					// record actual update rate in images
			jci[i].crypted=FALSE;
			jci[i].sipaddr[0]=0;						// server fills this in
			jci[i].width=w;
			jci[i].height=h;
			jci[i].cam=jcs.cam;
			jci[i].img=i;
			jci[i].optional_crc32=0;					// jc_msg_image() will add it if needed

			if ( (sendnow==TRUE) & (jcs.scaledown[0]==TRUE) )		// if we have a valid config
			{
				if (jcs.scaledown[i]==TRUE)
				{
					jc_msg_image(&ky, &jci[i], fdsock, jcs.docrc, reqid); // send image
					lastsendtime=current_timems();

					//print_struct_jcamsys_image(&jci[i]);	 
					//printf("\n");
				}
			}

			if (i!=smallest)
				scaleimage(0,(char*)&image_raw,&w,&h,1);		// image now 1/2 its previous size
		//}
	}
}





int read_frame(void)
{
	struct v4l2_buffer buf;
	static int frame_number=0;
int newest=0;

	v4lfc++;
  	CLEAR(buf);
  	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  	buf.memory = V4L2_MEMORY_MMAP;

  	//if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) 
	//{
   		//switch (errno) 
		//{
   			//case EAGAIN:
    				//return 0;

   			//case EIO:
    				/* Could ignore EIO, see spec. */
    				/* fall through */

   			//default:
    			//errno_exit("VIDIOC_DQBUF");
   		//}
  	//}


	// Read newest frame, seems to give half frame rate on Pi but full frame rate on webcam?
	newest=0;
	do
	{
  		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) 
   		switch (errno) 
		{
   			case EAGAIN:
				newest=1;
			break;
		}
		v4lfc++;
	} while (!newest);

  	assert(buf.index < n_buffers);

	if (fdsock>0)
  		v4l_process_image(useyuv,fmt.fmt.pix.width, fmt.fmt.pix.height, buffers[buf.index].start, buf.bytesused, frame_number++);
  	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
   		errno_exit("VIDIOC_QBUF");
 	return 1;
}






void start_capturing(void)
{
 	unsigned int i;
 	enum v4l2_buf_type type;
   	struct v4l2_buffer buf;

 	switch (io) 
	{
 		case IO_METHOD_MMAP:
  			for (i = 0; i < n_buffers; ++i) 
			{
   				//struct v4l2_buffer buf;
   				CLEAR(buf);
   				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   				buf.memory = V4L2_MEMORY_MMAP;
   				buf.index = i;

   				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    				errno_exit("VIDIOC_QBUF");
  			}
  			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  			if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
   			errno_exit("VIDIOC_STREAMON");
  		break;
		case IO_METHOD_READ:
		break;
		case IO_METHOD_USERPTR:
		break;
 	}
}



void uninit_device(void)
{
 	unsigned int i;

 	switch (io) 
	{
 		case IO_METHOD_MMAP:
  			for (i = 0; i < n_buffers; ++i)
   			if (-1 == munmap(buffers[i].start, buffers[i].length))
    				errno_exit("munmap");
  		break;

 		case IO_METHOD_USERPTR:
  			for (i = 0; i < n_buffers; ++i)
   			free(buffers[i].start);
  		break;

 		case IO_METHOD_READ:
		break;
 	}
 	free(buffers);
}


void init_read(unsigned int buffer_size)
{
 	buffers = calloc(1, sizeof(*buffers));

 	if (!buffers) {
  		fprintf(stderr, "Out of memory\\n");
  		exit(EXIT_FAILURE);
 	}

 	buffers[0].length = buffer_size;
 	buffers[0].start = malloc(buffer_size);

 	if (!buffers[0].start) {
  		fprintf(stderr, "Out of memory\\n");
  		exit(EXIT_FAILURE);
 	}
}



void init_mmap(void)
{
 	struct v4l2_requestbuffers req;

 	CLEAR(req);
 	req.count = 4;
 	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 	req.memory = V4L2_MEMORY_MMAP;

 	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) 
	{
  		if (EINVAL == errno) 
		{
   			fprintf(stderr, "%s does not support memory mappingn", dev_name);
   			exit(EXIT_FAILURE);
  		} else 
		{
   			errno_exit("VIDIOC_REQBUFS");
  		}
 	}

 	if (req.count < 2) 
	{
  		fprintf(stderr, "Insufficient buffer memory on %s\n",dev_name);
  		exit(EXIT_FAILURE);
 	}

 	buffers = calloc(req.count, sizeof(*buffers));

 	if (!buffers) {
  		fprintf(stderr, "Out of memory\\n");
  		exit(EXIT_FAILURE);
 	}

 	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{
  		struct v4l2_buffer buf;

  		CLEAR(buf);

  		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  		buf.memory      = V4L2_MEMORY_MMAP;
  		buf.index       = n_buffers;

  		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
   			errno_exit("VIDIOC_QUERYBUF");

  		buffers[n_buffers].length = buf.length;
  		buffers[n_buffers].start =
   		mmap(NULL /* start anywhere */,
  		buf.length,
  		PROT_READ | PROT_WRITE /* required */,
  		MAP_SHARED /* recommended */,
  		fd, buf.m.offset);

  		if (MAP_FAILED == buffers[n_buffers].start)
   			errno_exit("mmap");
 	}
}



void init_device(int useyuv, int initw, int inith)
{
 	struct v4l2_capability cap;
 	unsigned int min;
	
	printf("%s(%05d): init device %d %d %d\n",PROGNAME,getpid(),useyuv,initw,inith); fflush(stdout);
 	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) 
	{
  		if (EINVAL == errno) 
		{
   			fprintf(stderr, "%s is no V4L2 device\\n",dev_name);
   			exit(EXIT_FAILURE);
  		} else 
		{
   			errno_exit("VIDIOC_QUERYCAP");
  		}
 	}

 	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
	{
  		fprintf(stderr, "%s is no video capture device\\n",dev_name);
  		exit(EXIT_FAILURE);
 	}

 	switch (io) 
	{
 		case IO_METHOD_MMAP:
 		case IO_METHOD_USERPTR:
  		if (!(cap.capabilities & V4L2_CAP_STREAMING)) 
		{
   			fprintf(stderr, "%s does not support streaming i/o\\n",dev_name);
   			exit(EXIT_FAILURE);
  		}
  		break;
		case IO_METHOD_READ:
		break;
 	}


 	CLEAR(fmt);

 	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  	fmt.fmt.pix.width       = initw;
  	fmt.fmt.pix.height      = inith;
	if (useyuv==TRUE)
  		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  	else	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
  	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
  	//fmt.fmt.pix.field       = V4L2_FIELD_NONE;

  	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
   		errno_exit("VIDIOC_S_FMT");


 	/* Buggy driver paranoia. */
 	min = fmt.fmt.pix.width * 2;
 	if (fmt.fmt.pix.bytesperline < min)
  		fmt.fmt.pix.bytesperline = min;
 	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
 	if (fmt.fmt.pix.sizeimage < min)
  		fmt.fmt.pix.sizeimage = min;


	// Try to set a frame rate - needs work
	struct v4l2_streamparm parm;
    	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	parm.parm.capture.timeperframe.numerator = 1;
    	parm.parm.capture.timeperframe.denominator = 30;
	if (xioctl(fd, VIDIOC_S_PARM, &parm) < 0)
		printf("v4l ignored attempt to set frame rate\n");


	printf("%s(%05d): Asked V4L for %dx%d, got %dx%d\n",PROGNAME,getpid(),initw,inith,fmt.fmt.pix.width,fmt.fmt.pix.height);
	fflush(stdout);

	send_camerasettings();
 	switch (io) 
	{
 		case IO_METHOD_MMAP:
  			init_mmap();
  		break;
 		case IO_METHOD_READ:
		break;
		case IO_METHOD_USERPTR:
		break;
 	}
}



void close_device(void)
{
 	if (-1 == close(fd))
  		errno_exit("close");
 	fd = -1;
}


void open_device(void)
{
 	struct stat st;

 	if (-1 == stat(dev_name, &st)) 
	{
  		fprintf(stderr, "Cannot identify '%s': %d, %s\\n",dev_name, errno, strerror(errno));
  		exit(EXIT_FAILURE);
 	}

 	if (!S_ISCHR(st.st_mode)) 
	{
  		fprintf(stderr, "%s is no devicen", dev_name);
  		exit(EXIT_FAILURE);
 	}

 	fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
 	if (-1 == fd) 
	{
  		fprintf(stderr, "Cannot open '%s': %d, %s\\n",dev_name, errno, strerror(errno));
  		exit(EXIT_FAILURE);
 	}
}



void capture_init(int useyuv, int initw, int inith)
{
	dev_name = "/dev/video0";
	open_device();
	init_device(useyuv, initw, inith);
	start_capturing();
}


void capture_close()
{
	//printf("did close\n");
	//stop_capturing();
 	uninit_device();
	close_device();
}





int main( int argc, char *argv[ ] )
{
	char cmstring[1024];
	int q=FALSE;
	int i=0;
	int l=0;
	int r=0;
	int op_help=FALSE;
	int argnum=0;
	char st[2048];
	int trc=100;
	float pitemp;

        srand(time(NULL));
	client_settings_defaults(&stngs);
	//signal(SIGINT, sig_handler);
	//signal(SIGPIPE, socketclosed);
	signal(SIGPIPE, SIG_IGN);
    	bzero(&camserver_ipaddr,sizeof(camserver_ipaddr));
	bzero(&sen,sizeof(struct jcamsys_sensorstate));
	bzero(&dts,sizeof(dts));
	bzero(&jcs,sizeof(struct jcamsys_camerasettings));
	update_messagemask(&mm);
	jc_camera_default(&jcs,0);


	// Check if on an RPI board
	bzero(&st,sizeof(st));
	run_command("cat /proc/cpuinfo |grep 'BCM'", (char*)&st, sizeof(st));
	if (strlen(st)>0)
	{
		rpi=TRUE;
		system("modprobe bcm2835-v4l2");
		system("echo 21 >/sys/class/gpio/export");
		system("echo out >/sys/class/gpio/gpio21/direction");
	}
	

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
			strcpy(stngs.server_ipaddr,st);
			stngs.discover_server=FALSE;
		}
		else
		{
			printf("-host <host name or ipv4 address>\n");
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
	keylen=jcam_read_keyfile(stngs.filename_shared_key,&ky);
	if (keylen<0)
	{
		//printf("shared key [%s] missing or bad, use jcmakekey\n",stngs.filename_shared_key);
		//exit(1);
		while (q!=TRUE)
		{
			if (stngs.discover_server==TRUE)
			{
				bzero(&stngs.server_ipaddr,sizeof(stngs.server_ipaddr));
				printf("%s(%05d): trying to YAFDP discover server - ",PROGNAME,getpid());
				fflush(stdout);
				r=jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port,FALSE,TRUE);
				if (r==1)					// found server
				{
					i=jcam_key_exchange((char*)&ky.xorkey, JC_MAX_KEYSIZE, (char*)&stngs.server_ipaddr, stngs.server_tcp_port+10);
					if (i>10)
					{
						printf("got key, length=%d\n",i);
						fflush(stdout);
						ky.keylength=i;
						jcam_write_keyfile(stngs.filename_shared_key,&ky);
						q=TRUE;
					}
				}
			}
			else
			{
			}
			
			sleep(1);
		}
		keylen=jcam_read_keyfile(stngs.filename_shared_key,&ky);
	}
	else	printf("%s(%05d): got valid key, length=%d\n",PROGNAME,getpid(),keylen);

	// Crypt plain text password into non plain text using key
	pwlen=jcam_crypt_buf(&ky,(char*)&pw,sizeof(pw),pwlen);

	bzero(&registration,sizeof(registration));
	strcat(stngs.filename_client_registration,"_cam");

	if (loadfile(stngs.filename_client_registration,(char*)&registration,sizeof(struct jcamsys_register))==JC_ERR)
		printf("%s(%05d): failed to load id, that is ok, the server should supply one\n",PROGNAME,getpid());

	printf("%s(%05d): starting, my id:%s\n",PROGNAME,getpid(),registration.id);
	fflush(stdout);

   	fd_set fds;
   	struct timeval tv;
	int sec=0;
	int psec=0;

 	//count = frame_count;

	while (quit!=TRUE)
	{
		for (i=0;i<10;i++)
			jc_events();
		// With luck we can close, and init/reinit capture
		if ( (jcs.requested_width != prequested_width) | (jcs.requested_height != prequested_height) )
		{
			prequested_width=jcs.requested_width;
			prequested_height=jcs.requested_height;
			reinit=TRUE;
		}
		if (reinit==TRUE)
		{
			if (fd>0)
				capture_close();
			printf("%s(%05d): Doing capture_init()\n",PROGNAME,getpid());
			capture_init(useyuv, jcs.requested_width, jcs.requested_height);
			reinit=FALSE;
		}


		sec=get_clockseconds();
		if (sec!=psec)
		{
			psec=sec;
			actualfps=framecount;
			if (verbose!=TRUE)
			{
				if (fdsock>0)
				printf("%s(%05d): cam=%d\tDTS[%s]\tV4Lfps=%d\tactualfps=%d\t\n",PROGNAME,getpid(),jcs.cam,dts,v4lfc,actualfps);
				fflush(stdout);
			}
			framecount=0;
			v4lfps=v4lfc;
			v4lfc=0;

			trc++;
			if ( trc>30 )
			{
				trc=0;
				if (rpi==TRUE)
				{
					pitemp=rpi_read_soc_temp();
					printf("****pi temp=%2.1fc****\n",pitemp);

					// send it
					jc_msg_sens_value(&ky, fdsock, 0, JC_SENSTYPE_CAMERA_TEMP, pitemp, 0, "");
				}
			}	


			//r=1000/(float)actualfps;			// work out the current iner frame delay in ms
			//if ( (r > jcs.update_rate_ms) & (msi>2) )		// running slow ?
				//msi--;						// reduce the value of msi
		}


   		FD_ZERO(&fds);
   		FD_SET(fd, &fds);

   		/* Timeout. */
   		tv.tv_sec = 2;
   		tv.tv_usec = 0;
   		r = select(fd + 1, &fds, NULL, NULL, &tv);
   		if (-1 == r) 
		{
    			if (EINTR == errno)
	    			continue;
    			errno_exit("select");
   		}

   		if (0 == r) 
		{
    			fprintf(stderr, "select timeout\\n");
    			exit(EXIT_FAILURE);
   		}

   		read_frame();
 	}
	return(0);
}



