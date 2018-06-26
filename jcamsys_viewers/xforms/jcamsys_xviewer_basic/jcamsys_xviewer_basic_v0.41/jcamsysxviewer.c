// Xforms camera viewer for X11 displays
// 
// printf() will crash the program if running long term.  Only the connection phase has any
// printfs now, hopefully that will make it long term stable.
// jpeg decoding is done by a second forked process with shared memory, this allows another
// child to be started if the jpeg decoder segfaults but it also boosts performance as
// a second CPU is able to handle decode.
//


// TODO: Timeout on starting jpeg decoder


 
#define PROGNAME	"jcamsysxviewer"
#define VERSION         "0.41"

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
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
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <math.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/wait.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <stropts.h>
#include <sgtty.h>

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#include "jcamsysxviewer_form.h"
#include <flimage.h>
FL_IMAGE	*camimage = NULL;

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_statistics.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_protocol.h"
//#include "jcamsys_sharedmem.h"
#include "jcamsys_client_settings.h"
#include "jcamsys_network.h"
#include "jcamsys_images.h"
#include "id.h"


int debug=FALSE;
int verbose=FALSE;
char id[32];
int cp=0;		// child pid
char binname[512];

unsigned char msgbuffer[JC_MAX_MSGLEN];
struct jcamsys_header*          hdr  =(struct jcamsys_header*)&msgbuffer;

// requests
struct jcamsys_req_sens_info*   	reqn 	= (struct jcamsys_req_sens_info*)&msgbuffer;
struct jcamsys_req_sens_active* 	reqa 	= (struct jcamsys_req_sens_active*)&msgbuffer;
struct jcamsys_req_sens_sendme* 	reqs 	= (struct jcamsys_req_sens_sendme*)&msgbuffer;
struct jcamsys_req_sens_values*  	reqv 	= (struct jcamsys_req_sens_values*)&msgbuffer;
struct jcamsys_req_image*       	reqi 	= (struct jcamsys_req_image*)&msgbuffer;

// messages
struct jcamsys_header* 		     	msgh 	= (struct jcamsys_header*)&msgbuffer;
struct jcamsys_ack*         		msga	= (struct jcamsys_ack*)&msgbuffer;
struct jcamsys_image*       		msgi 	= (struct jcamsys_image*)&msgbuffer;
struct jcamsys_sens_info*   		msgf 	= (struct jcamsys_sens_info*)&msgbuffer;
struct jcamsys_sens_active* 		msgs 	= (struct jcamsys_sens_active*)&msgbuffer;
struct jcamsys_sens_values*  		msgv 	= (struct jcamsys_sens_values*)&msgbuffer;
struct jcamsys_frame_number*		msgn	= (struct jcamsys_frame_number*)&msgbuffer;


int sent_reqid=0;
int rx_reqid;						// IDs for images we received
int sent_request_time;					// time we made request
int image_received_time;
int decoded_time;					// time we finished decoding (and displaying) image
int ignore_framechange=0;

uint16_t frame_number[JC_MAX_IMAGES][JC_MAX_CAMS+1];
uint16_t pframe_number[JC_MAX_IMAGES][JC_MAX_CAMS+1];


struct sharedm
{
	struct jcamsys_image		im;	
	char				donow;
	char				doexit;
	char 				rgb[JC_MAX_WIDTH * JC_MAX_HEIGHT * 3];
	pid_t				childpid;
};
struct sharedm* shm	= NULL;				// will exist after mmap


struct jcamsys_client_settings		stngs;
struct jcamsys_key		ky;
struct jcamsys_sensorstate	sen;
struct jcamsys_messagemask	mm;						// local copy, set here then send to server
struct jcamsys_register         registration;


int keylen=0;
int fdsock=-1;
int pwlen=0;
char pw[JC_MAX_PASSWORD_LEN];
int gotcamchange=0;
FD_cameraview *fd_cameraview;
int selected_cam=1;
char camserver_ipaddr[32];
int preview_width=160;
int preview_height=120;

int full_width=640;
int full_height=480;
int pfull_width=0;
int pfull_height=0;
int formsize=1;

int last_image_request_time_ms=0;
int completed=TRUE;
int completed_time=0;
int slave_died=FALSE;


void jc_events();


// TODO:
// to get active cameras:
// use jcamsys_req_sens_active message with senstype=JC_SENSTYPE_CAMERA


void printver()
{
	printf("xforms version %d.%d.%s\n",FL_VERSION, FL_REVISION,FL_FIXLEVEL);
}



// signal handlder, when slave dies ask main() to launch another one
void slavedied()
{
	pid_t   p;
	//printf("slavedied()\n"); fflush(stdout);
	do
	{
		p=waitpid((pid_t)(-1), 0, WNOHANG);
		if (p==cp)									// our slave ?
                {
			//printf("p=%d cp=%d\n",p,cp);  fflush(stdout);
			slave_died=TRUE;
		}
	} while (p>0);
}



// Slave process, looks for a flag in shared memory, decodes jpeg, changes flag to say job done
int start_slave_jpeg_decoder()
{
	int oc=0;
	pid_t	p;

	shm->donow=FALSE;									// setup flags for child
	shm->doexit=FALSE;
	shm->childpid=0;

	p=fork();
	if (p==0)
	{
		printf("%s(%05d): started slave jpeg decoder\n",PROGNAME,getpid());
		fflush(stdout);

		shm->childpid=getpid();
		while (1)
		{
			while (shm->donow!=TRUE) 
			{
				usleep(100);
				if (shm->doexit==TRUE)
				{
					shm->doexit=FALSE;
					exit(0);
				}
			}
			decode_jpeg((unsigned char*)shm->im.cdata, shm->im.cbytes, (int*)&shm->im.width, (int*)&shm->im.height, &oc, (unsigned char*)shm->rgb);
			shm->donow=FALSE;
		}
	}

	while (shm->childpid==0)
	{
		//printf("w");
		//fflush(stdout);
		usleep(100);
	}
	signal(SIGCHLD,slavedied);
	return(shm->childpid);
}



// Update our local copy of the message mask
void update_messagemask(struct jcamsys_messagemask* mx, int cam)
{
	int i=0;

	for (i=0;i<JC_MAX_CAMS+1;i++)
	{
		mx->frame_changes[IFULL][i]=FALSE;						// start will all frame change messages disabled
		mx->camera_settings[i]=FALSE;
	}
	mx->frame_changes[IFULL][cam]=TRUE;							// turn on one camera
	mx->sensors=TRUE;									// we want sensor data
	mx->dts=FALSE;										// dont want date time strings sent to us
	mx->comment=FALSE;
}



void reload_image(unsigned char *jpg_buffer, int jpg_size)
{
	int ih=0, iw=0;
	int tout=0;
	int w,h;

	shm->donow=TRUE;									// trigger slave to decode
	tout=1000;
	do
	{
		if (shm->donow==TRUE)
			usleep(1000);
		tout--;

	} while (( shm->donow==TRUE) & (tout>0) );						// wait for it to complete
	if ( (tout==0) | (shm->im.width==0) | (shm->im.height==0) )				// timed out, ignore image
	{
		printf("%s(%05d): timeout waiting for donow!=TRUE\n",PROGNAME,getpid());
		fflush(stdout);
		return;
	}

	//printf("decoded jpeg, got %d x %d, %d bytes RGB\n",shm->im.width,shm->im.height,shm->im.cbytes); 
	//fflush(stdout);
	if (camimage==NULL)
		camimage = flimage_alloc();

	if (formsize==0)
	{
		w=shm->im.width;
		h=shm->im.height;
		scaleimage(0,(char*)shm->rgb,&w,&h,1);
		shm->im.height=h;
		shm->im.width=w;
	}

	camimage->type = FL_IMAGE_RGB;
    	camimage->w = shm->im.width;
    	camimage->h = shm->im.height;
    	camimage->map_len = 1;
    	flimage_getmem(camimage); 

	// copy decoded jpeg into FL_IMAGE
	int p=0;
	for (ih=0;ih<shm->im.height;ih++)
	{
		for (iw=0;iw<shm->im.width;iw++)
		{
			camimage->red  [ih][iw] = shm->rgb[p];
			camimage->green[ih][iw] = shm->rgb[p+1];
			camimage->blue [ih][iw] = shm->rgb[p+2];
			p=p+3;
		}	
	}	
	//camimage->modified=1;
	//flimage_display( camimage, FL_ObjWin( fd_cameraview->largeviewcanvas ) );
	flimage_sdisplay( camimage, FL_ObjWin( fd_cameraview->largeviewcanvas ) );
	flimage_free(camimage);
	camimage=NULL;
}


// If on a LAN then the reply may be pretty fast, rather than wait for the next timer to fire have
// a go at getting the reply
void dwell_event_poll(int timeoutms)
{
	int tms=timeoutms;

	do
	{
		sleep_ms(1);
		jc_events();
		tms--;
	} while (tms>0);
}



// Sleep N seconds, but keep the GUI alive while we do it
void formsleep(int s)
{
	int i=0;
	for (i=0;i<s*1000;i++)
	{
		fl_check_forms();
		sleep_ms(1);
	}
}


void do_image_request(int img, int cam, int x)
{
	if (fdsock<0)
		return;

	if (completed==TRUE)										// only ask for a new image if we have the requested one
	{
		completed=FALSE;									// only ask for a new image if we have the requested one
		sent_request_time=current_timems();
		sent_reqid = jc_request_image(&ky, fdsock, 0, IFULL, selected_cam);			// Record the reqid we passed to server
//printf("%d asked for an image, request id = %d\n",x,sent_reqid); fflush(stdout);
		last_image_request_time_ms=current_timems();
		if (sent_reqid==-1)
		{ 
			printf("socket closed?\n"); fflush(stdout);
			close(fdsock);
			fdsock=-1;
			return;
		}
	}
}





#define LCH		FL_WHITE
//#define LCU		FL_COL1
#define LCU		FL_BOTTOM_BCOL		// Dark grey
// Set label color to bright if camera is active, dim if not
void highlight_active_cams()
{
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][1]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam1, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam1, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][2]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam2, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam2, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][3]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam3, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam3, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][4]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam4, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam4, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][5]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam5, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam5, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][6]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam6, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam6, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][7]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam7, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam7, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][8]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam8, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam8, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][9]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam9, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam9, LCU);
   	if (sen.sensor_active[JC_SENSTYPE_CAMERA][10]==TRUE)
		fl_set_object_lcolor (fd_cameraview->bcam10, LCH);
	else 	fl_set_object_lcolor (fd_cameraview->bcam10, LCU);
}



#define HIGHLIGHT	FL_DARKGREEN
#define HB		FL_BLACK
void highlight_button( int b )
{
	if (verbose==TRUE)
		printf("%s(%05d): Changed to camera %d\n",PROGNAME,getpid(),b);
	fl_set_object_color( fd_cameraview->bcam1 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam2 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam3 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam4 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam5 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam6 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam7 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam8 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam9 , FL_BLACK, FL_BLACK );
	fl_set_object_color( fd_cameraview->bcam10 , FL_BLACK, FL_BLACK );
	switch(b)
	{
		case 1:   fl_set_object_color( fd_cameraview->bcam1 , HIGHLIGHT, HB );  break;
		case 2:   fl_set_object_color( fd_cameraview->bcam2 , HIGHLIGHT, HB );  break;
		case 3:   fl_set_object_color( fd_cameraview->bcam3 , HIGHLIGHT, HB );  break;
		case 4:   fl_set_object_color( fd_cameraview->bcam4 , HIGHLIGHT, HB );  break;
		case 5:   fl_set_object_color( fd_cameraview->bcam5 , HIGHLIGHT, HB );  break;
		case 6:   fl_set_object_color( fd_cameraview->bcam6 , HIGHLIGHT, HB );  break;
		case 7:   fl_set_object_color( fd_cameraview->bcam7 , HIGHLIGHT, HB );  break;
		case 8:   fl_set_object_color( fd_cameraview->bcam8 , HIGHLIGHT, HB );  break;
		case 9:   fl_set_object_color( fd_cameraview->bcam9 , HIGHLIGHT, HB );  break;
		case 10:  fl_set_object_color( fd_cameraview->bcam10, HIGHLIGHT, HB );  break;
	}
        selected_cam=b;
	update_messagemask(&mm, selected_cam);
	//printmm(&mm);
	//printf("\n");

	jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);
	//sleep_ms(2);
	completed=TRUE;
	do_image_request(IFULL,selected_cam,10);
}



void sizechange()
{
	if (formsize==1)
		fl_winresize(FL_ObjWin(fd_cameraview->largeviewcanvas),full_width,full_height+(full_height/12));
	else	fl_winresize(FL_ObjWin(fd_cameraview->largeviewcanvas),full_width/2,(full_height/2)+(full_height/(12*2)));
	image_received_time=current_timems()-100000;				// will force image fetch
}


// Button push passes l=0,
void changesize( FL_OBJECT *obj, long l )
{
	if (l==100)
		formsize=1;
	else	formsize=!formsize;
	sizechange();

	if (verbose==TRUE)
		printf("%s(%05d): formsize= %d\n",PROGNAME,getpid(),formsize);
}




void jc_message_parser(int msgtype, uint16_t reqid)
{
	int i=0;
	uint32_t  crc=0;
	uint32_t  ct=0;

	switch (msgtype)							// what type of message ?
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
				printf("%s(%05d): got JC_REQ_IMAGE %d\n",PROGNAME,getpid(),reqid);
		break;


		case JC_MSG_IMAGE:
			image_received_time=current_timems();
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_IMAGE\n",PROGNAME,getpid());

			if (valid_image_cam(msgi->img, msgi->cam, PROGNAME, "\0", FALSE))
				return;
			//if ( (msgi->cam > JC_MAX_CAMS+1 ) | ( msgi->cam==0) )
			//{
				//printf("\n\nmsg_image: cam=%d out of range\n",msgi->cam);
				//completed=TRUE;								// we lost an image, so pretend cycle completed
				//return;
			//}
			//if (msgi->img > JC_MAX_IMAGES-1)
			//{
				//printf("\n\nmsg_image img=%d out of range\n",msgi->img);
				//completed=TRUE;								// we lost an image, so pretend cycle completed
				//return;
			//}

			if (msgi->crypted==TRUE)
			{
				if (verbose==TRUE)
				{
					printf("%s(%05d): got ",PROGNAME,getpid());
					printf("encrypted image reqid=%d img=%d cam=%d  %dx%d size=%d\n",
						reqid,msgi->img,msgi->cam,msgi->width,msgi->height,msgi->cbytes);
				}
				jcam_crypt_buf(&ky, (char*) msgi->cdata, sizeof(msgi->cdata), msgi->cbytes);	// decrypt it
			}
			else	
			{
				if (verbose==TRUE)
					printf("uncrypted image reqid=%d img=%d cam=%d  %dx%d size=%d\n",
						reqid,msgi->img,msgi->cam,msgi->width,msgi->height,msgi->cbytes);
			}

			if (msgi->optional_crc32!=0)
			{
				crc=rc_crc32(0,msgi->cdata, msgi->cbytes);
				//printf("%08X == %08X\n",msgi->optional_crc32,crc); fflush(stdout);
				if (msgi->optional_crc32!=crc)
				{
					completed=TRUE;							// ignoring this image, need to fetch a new one
					printf("%d:FAIL FN:%06d CRC:%08X OURCRC:%08X\n",getpid(),msgi->frame_number,msgi->optional_crc32,crc);
					fflush(stdout);
					return;								// dont use this image
				}
			}
			//print_struct_jcamsys_image(msgi);
			jc_image_copy(&shm->im, msgi);


			if (verbose==TRUE)
			{
				ct=current_timems();
				if (sent_reqid==reqid)
				{
					printf("%s(%05d): request to arrival (%dms) ",PROGNAME,getpid(),ct-sent_request_time);
					printf("+ decode&render (%dms)",image_received_time-decoded_time);
					printf("\t= %dms\n",(ct-sent_request_time)+(image_received_time-decoded_time));
					fflush(stdout);
				}
			}
			//printf("B:"); DumpHex(msgi->data,10);						// Beginning of buf
			//printf("E:"); DumpHex(msgi->data+(msgi->cbytes-18),18);

			if (msgi->cam==selected_cam)
			{
				full_width=msgi->width;
				full_height=msgi->height;
				if ( (pfull_width!=full_width) | (pfull_height!=full_height) )
				{
					pfull_width=full_width;
					pfull_height=full_height;
					sizechange();
				reload_image((unsigned char*)msgi->cdata, msgi->cbytes);
				}

				reload_image((unsigned char*)msgi->cdata, msgi->cbytes);
				decoded_time=current_timems();
			}
			if (sent_reqid == reqid)							// did we get the image we asked for 
			{
				sent_reqid=0;
				completed=TRUE;								// unlock for another request
				completed_time=current_timems();
			}
			else	
			{
				if (verbose==TRUE)
				{
					printf("Got image with reqid=%d, expecting %d\n",reqid,sent_reqid);
					fflush(stdout);
				}
			}
		break;


		case JC_MSG_SENS_ACTIVE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_ACTIVE\n",PROGNAME,getpid());
			if (msgs->senstype>=JC_MAX_SENSTYPE-1)
				return;
			for (i=0;i<=JC_MAX_SENSORS+1;i++)
				sen.sensor_active[msgs->senstype][i] = msgs->sensor_active[i];
			if (msgs->senstype==JC_SENSTYPE_CAMERA)
				highlight_active_cams();
		break;


		case JC_MSG_SENS_VALUES:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_VALUE\n",PROGNAME,getpid());

			if (msgv->senstype>=JC_MAX_SENSTYPE-1)
				return;
			for (i=0;i<JC_MAX_SENSORS+1;i++)
			{
				// copy into our local sensor state 
				sen.sensor_fvalue[msgv->senstype][i] = msgv->sensor_fvalue[i];
				//if (sen.sensor_active[msgv->senstype][i]==TRUE)
					//printf("S:%d\tA:%d\tV:%02.1f\n",i, sen.sensor_active[msgv->senstype][i], sen.sensor_fvalue[JC_SENSTYPE_TEMP][i]); 
			}
		break;


		// Signifies an image the server is holding has changed
		case JC_MSG_FRAME_NUMBER:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_FRAME_NUMBER\n",PROGNAME,getpid());
			if (verbose==TRUE)
			{
				printf("%s(%05d): img=%d cam=%d changed frameno=%d sc=%d \n",PROGNAME,getpid(),
					msgn->img, msgn->cam, msgn->frame_number, selected_cam);
				fflush(stdout);
			}
			if ( (msgn->cam > JC_MAX_CAMS+1 ) | ( msgn->cam==0) )
				return;
			pframe_number[msgn->img][msgn->cam] = frame_number[msgn->img][msgn->cam];		// save previous value
			frame_number[msgn->img][msgn->cam] = msgn->frame_number;				// record new value
			if ( (msgn->cam == selected_cam) & (msgn->img==IFULL) )					// is it for our display ?
					do_image_request(IFULL,selected_cam,11);				// then ask for a newer image from server
		break;


		case JC_MSG_REGISTER:
		break;
	}
}



#define JC_CSTATE_INITIAL		0
#define JC_CSTATE_FIND_SERVER		1
#define JC_CSTATE_CONNECT_AND_AUTH	2
#define JC_CSTATE_RUNNING		3
#define JC_CSTATE_PAUSE_THEN_INITIAL	10


void jc_events()
{
	static int state=0;
	char st[2048];
	int i=0;
	int r=0;
	int rms=0;
	//static unsigned int last_image_request_time_ms=0;
	unsigned int time_ms=0;
	static int timer_initial_ms=0;
	static int timer_duration_ms=0;
	int tdiff=0;

	int msgtype=0;
	uint16_t reqid=0;
	uint64_t sendtimems=0;

	fl_check_forms();
	time_ms=current_timems();

	if ( ignore_framechange >0 )
		ignore_framechange--;


	switch (state)
	{
		case JC_CSTATE_INITIAL:
			//fl_set_object_label(fd_cameraview->textbox,"Hello");
			bzero(&sen,sizeof(struct jcamsys_sensorstate));						// blank sensor state whenever disconnected
			highlight_active_cams();								// show cameras have vanished if disconnected
			state++;
			if (stngs.discover_server!=TRUE)							// -host specified the server
				state=JC_CSTATE_CONNECT_AND_AUTH;						// skip over find_server
		break;

		case JC_CSTATE_FIND_SERVER:									// go through this state even if -host 
			if (stngs.discover_server==TRUE)
			{
				fl_set_object_label(fd_cameraview->textbox,"trying to YAFDP discover server");
				fl_show_object(fd_cameraview->textbox);
				fl_check_forms();
				//fl_redraw_object(fd_cameraview->textbox);
				bzero(&stngs.server_ipaddr,sizeof(stngs.server_ipaddr));
				printf("%s(%05d): trying to YAFDP discover server - ",PROGNAME,getpid());
				fflush(stdout);
				r=jc_discover_server((char*)&stngs.server_ipaddr,&stngs.server_tcp_port,TRUE,TRUE);
				switch (r)
				{
					case -2:
						if (verbose==TRUE)
						printf("discover service busy\n");
						fflush(stdout);
						fl_set_object_label(fd_cameraview->textbox,"YAFDP discover service busy");
						return;
                                        break;

					case -1:
						if (verbose==TRUE)
						printf("yafdp error of some sort\n");
						fflush(stdout);
						fl_set_object_label(fd_cameraview->textbox,"YAFDP discover err");
						return;
                                        break;
				}
				if (strlen(stngs.server_ipaddr)<6)
				{
					fl_set_object_label(fd_cameraview->textbox,"trying to YAFDP discover server - discover failed");
					//fl_redraw_object(fd_cameraview->textbox);
					fl_check_forms();
					printf("discover failed\n");
					fflush(stdout);
					timer_duration_ms= 2 * 1000;
					state=JC_CSTATE_PAUSE_THEN_INITIAL;
				}
				else    
				{
					printf("found server\n");
					state=JC_CSTATE_CONNECT_AND_AUTH;
					return;
				}
				formsleep(3);
			}
			if (  (strlen(stngs.server_ipaddr)<6) & (stngs.discover_server!=TRUE) )		// no IP supplied and not discovering one
			{
				printf("%s(%05d): need to either discover or run with -host ",PROGNAME,getpid());
				fflush(stdout);
				exit(1);
			}
		break;


		case JC_CSTATE_CONNECT_AND_AUTH:
			sprintf(st,"connecting to server %s port %d",stngs.server_ipaddr,stngs.server_tcp_port);
			fl_set_object_label(fd_cameraview->textbox,st);
			fl_redraw_object(fd_cameraview->textbox);
			printf("%s(%05d): connecting to server host=%s ip=%s port=%d\n",PROGNAME,getpid(),
                                stngs.server_hostname,stngs.server_ipaddr,stngs.server_tcp_port);
			fflush(stdout);
                        fdsock=jc_connect_to_server(stngs.server_ipaddr,stngs.server_tcp_port,FALSE);
			//printf("****** fdsock=%d\n",fdsock);
                        if (fdsock<0)
                        {
				fdsock=-1;
                                printf("%s(%05d): connect failed\n",PROGNAME,getpid());
				fl_set_object_label(fd_cameraview->textbox,"connect failed");
				fl_check_forms();
				//fl_redraw_object(fd_cameraview->textbox);
                                if (stngs.persistent_connect!=TRUE)
				{
					printf("exit 1\n");
                                        exit(1);
				}
				formsleep(1);
				state=JC_CSTATE_INITIAL;
			}
                        else										// socket is connected
                        {
                                printf("%s(%05d): connected, doing auth : ",PROGNAME,getpid());
                                //i=jcam_crypt_buf(&ky,(char*)&pw,sizeof(pw),pwlen);
                                i=jcam_authenticate_with_server(fdsock, (char*)&pw, pwlen, 5000);
				if (i==JC_OK)
				{
					printf("auth ok\n"); fflush(stdout);
					completed=TRUE;							// so we can send a request for image
					update_messagemask(&mm, selected_cam);
					jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);
					image_received_time=time_ms-100000;				// will force image fetch
					state=JC_CSTATE_RUNNING;
					fl_hide_object(fd_cameraview->textbox);
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
						exit(1);
					fflush(stdout);
					formsleep(1);
					state=0;
				}
                        }
		break;


		case JC_CSTATE_RUNNING:									// connected and processing messages
			rms=jcam_read_message(fdsock, &ky, (unsigned char*)&msgbuffer, sizeof(msgbuffer), &msgtype, &reqid, &sendtimems, 1);
			//printf("rms=%d  msgtype=%d\n",rms,msgtype); fflush(stdout);
			if (rms==JC_HAVE_DATA)
				jc_message_parser(msgtype, reqid);
			if (rms==-1)
			{
				close(fdsock);
				fdsock=-1;
				state=0;
				fl_set_object_label(fd_cameraview->textbox,"Disconnected");
				fl_show_object(fd_cameraview->textbox);
			}	

			tdiff=time_ms-image_received_time;						// time now  - time last image arrived
			if ( tdiff > 1000)								// More than a second single last image ?
			{
				completed=TRUE;
				do_image_request(IFULL,selected_cam,13);
			}

			if (slave_died==TRUE)
			{
				slave_died=FALSE;
				printf("%s(%05d): slave decoder %d died\n",PROGNAME,getpid(),cp);
				fflush(stdout);
				cp=start_slave_jpeg_decoder();
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




// Run once a second, or more frequently if called from other functions
void timer_cb (FL_OBJECT* obj, long data )
{
	//printf("timer\n");	fflush(stdout);
	dwell_event_poll(2);
	//jc_events();
	fl_check_forms();
	fl_set_timer(fd_cameraview->tim, 0.01);
}



void idle_callback()
{
	//printf("idle\n");	fflush(stdout);
	dwell_event_poll(2);
}

void camchange( FL_OBJECT* obj, long data )
{
	highlight_button(data);
	//timer_cb (fd_cameraview->bcam1,0);
}


void newme( FL_OBJECT* obj, long data )
{
	char cmdline[2048];

	if (fdsock<=0)
		return;

	sprintf(cmdline,"%s -host %s -p %d -not %d&",binname,stngs.server_ipaddr,stngs.server_tcp_port,selected_cam);
printf("%s\n",cmdline); fflush(stdout);

	// Start a new copy of ourself, but this time we can skip discovery
	system(cmdline);
}

void sigpipe()
{
	printf("Broken pipe\n"); 
	exit(1);
}


void killed_cb()
{
	while (shm->doexit==TRUE) 
	{
		printf("W");
		fflush(stdout);
	}

	printf("cleanup time\n"); fflush(stdout);
	if (cp>0)
		kill(cp,SIGKILL);
	munmap(shm,sizeof(shm));
    	fl_free( fd_cameraview );
    	fl_finish( );
	exit(0);
}


FD_cameraview *fd_cameraview;
int main( int argc, char *argv[ ] )
{
	char cmstring[1024];
	int i=0;
	int l=0;
	int op_help=FALSE;
	int argnum=0;
	char st[2048];

	strcpy(binname,argv[0]);
	srand(time(NULL));
	client_settings_defaults(&stngs);
    	bzero(&camserver_ipaddr,sizeof(camserver_ipaddr));
	bzero(&sen,sizeof(struct jcamsys_sensorstate));
	strcpy(id,JCID);


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
		printf(" -v\t\t\tVerbose - show messages\n");
		printf(" -host <name/ip>\tDont try discovery, just connect to this server\n");
		printf(" -debug\t\t\tDebug output useful for testing\n");
		printf(" -1\t\t\tTry to connect with server once, exit if it fails\n");
		printf(" -cam <cam>\t\tThe camera we want\n");
		printf(" -preview\t\tFetch the smaller thumbnail image\n");
		printf(" -fps <num>\t\tKeep fething images at this rate\n");
		printf(" -not <num>\t\tDisplay next active cam that is not this one\n");
                exit(0);
        }

	i=0;
	strcpy(cmstring,"-not");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		argnum=parse_findargument(argc,argv,cmstring);
		if (argnum>0)
		{
			strcpy(st,argv[argnum]);
			if (strlen(st)<1)
			{
				printf("-not <value>\n");
				//exit(1);
			}
			else    i=atoi(st);
		}
	}
	if (i>0)									// we dont have a list of active cams yet so just pick the next one
	{
		selected_cam=i+1;
		if (selected_cam>10)
			selected_cam=1;
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
		stngs.discover_server=FALSE;
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
	printf("%s(%05d): ",PROGNAME,getpid());
	printver();
	signal(SIGPIPE,SIG_IGN);
	keylen=jcam_read_keyfile(stngs.filename_shared_key,&ky);
	if (keylen<0)
	{
		printf("shared key [%s] missing or bad, use jcmakekey\n",stngs.filename_shared_key);
		exit(1);
	}
	else	printf("%s(%05d): got valid key, length=%d\n",PROGNAME,getpid(),keylen);
	// Crypt plain text password into non plain text using key
	jcam_crypt_buf(&ky,(char*)&pw,sizeof(pw),pwlen);

	// Shared memory buffer between this processes and slave_jpeg_decoder
	shm = mmap(NULL, sizeof(struct sharedm), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	cp=start_slave_jpeg_decoder();

    	fl_initialize( &argc, argv, 0, 0, 0 );
    	fd_cameraview = create_form_cameraview( );
	fl_set_atclose((FL_FORM_ATCLOSE)killed_cb, (void *) fd_cameraview->cameraview);
    	fl_show_form( fd_cameraview->cameraview, FL_PLACE_CENTERFREE, FL_FULLBORDER, "cameraview" );


	flimage_enable_jpeg();
    	highlight_button(selected_cam);
	fl_set_timer(fd_cameraview->tim, 1);
	fl_set_idle_callback( (FL_APPEVENT_CB)idle_callback, NULL );
	fl_show_object(fd_cameraview->textbox);
	sprintf(st,"%s version %s\n(c)2018 J.Andrews (jon@jonshouse.co.uk)\n",PROGNAME,VERSION);
	fl_set_object_label(fd_cameraview->textbox,st);
    	fl_do_forms();

    	if ( fl_form_is_visible( fd_cameraview->cameraview ) )
        	fl_hide_form( fd_cameraview->cameraview );
    	fl_free( fd_cameraview );
    	fl_finish( );
    	return 0;
}






