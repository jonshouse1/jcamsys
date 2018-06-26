// 
#define PROGNAME	"jcamsysxconsole"
#define VERSION         "0.10"

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

#include "jcamsys.h"
#include "jcamsysxconsole_form_main.h"
#include "jcamsysxconsole_form_camerasettings.h"
#include "jcamsysxconsole_form_serversettings.h"
#include <flimage.h>

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_client_settings.h"
#include "jcamsys_network.h"
#include "jcamsys_images.h"
#include "jcamsysxconsole.h"


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
struct jcamsys_statistics*		msgst	= (struct jcamsys_statistics*)&msgbuffer;
struct jcamsys_camerasettings*          msgcs	= (struct jcamsys_camerasettings*)&msgbuffer;
struct jcamsys_comment*			msgcm	= (struct jcamsys_comment*)&msgbuffer;


int state=0;
int sent_reqid=0;
int rx_reqid;						// IDs for images we received
int sent_request_time;					// time we made request
int image_received_time=0;
int decoded_time;					// time we finished decoding (and displaying) image
int ignore_framechange=0;
int force_request=FALSE;
int doclear=FALSE;					// clear image buffers before jpeg decode
int sendcamsettings=FALSE;
int sendserversettings=FALSE;

// Arrival time
int 	sc_takesample=TRUE;				// look at next frame of data from server and calc time difference
int	psc_time_diff_ms;
int	sc_time_diff_ms;				// Server, client time difference in ms


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
struct jcamsys_server_settings	se;
struct jcamsys_serverstate	ss;


int cs_changed[JC_MAX_CAMS+1];
struct jcamsys_camerasettings	cs[JC_MAX_CAMS+1];



// Xforms
int app_width=1024;
int app_height=741;
int image_width=0;								// Height of full size cameraview image
int image_height=0;

FL_IMAGE	*camimage = NULL;
FD_main		*fd_main = NULL;
FD_camerasettings* fd_camerasettings=NULL;
FD_serversettings* fd_serversettings=NULL;

int pcurrent_form=-1;
int current_form=1;
#define FORM_SERVERSETTINGS	1
#define FORM_CAMERASETTINGS	2


int keylen=0;
int fdsock=-1;
int pwlen=0;
char pw[JC_MAX_PASSWORD_LEN];
int gotcamchange=0;
int selected_cam=1;
//char camserver_ipaddr[32];
int preview_width=160;
int preview_height=120;

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


void dostop()
{
	signal(SIGINT,SIG_DFL);
	//printf("dostop\n"); fflush(stdout);
	shm->doexit=TRUE;
	sleep_ms(100);
	exit(0);
}


// signal handlder, when slave dies ask main() to launch another one
void slavedied()
{
	pid_t	p;
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
					printf("%s(%05d): slave jpeg decoder exit\n",PROGNAME,getpid());
					fflush(stdout);
					exit(0);
				}
			}
			decode_jpeg((unsigned char*)shm->im.cdata, shm->im.cbytes, (int*)&shm->im.width, (int*)&shm->im.height, &oc, (unsigned char*)shm->rgb);
			shm->donow=FALSE;
		}
	}
	signal(SIGCHLD,slavedied);
	signal(SIGINT,dostop);									// if interrupted we need to stop the child

	while (shm->childpid==0)
	{
		//printf("w");
		//fflush(stdout);
		usleep(50);
	}
	return(shm->childpid);
}



// Update our local copy of the message mask
void update_messagemask(struct jcamsys_messagemask* mx, int cam)
{
	int i=0;

	for (i=1;i<JC_MAX_CAMS+1;i++)
	{
		mx->frame_changes[IFULL][i]=FALSE;						// start will all frame change messages disabled
		mx->camera_settings[i]=TRUE;							// server should send us any camera settings change
	}
	if (cam!=0)
		mx->frame_changes[IFULL][cam]=TRUE;						// turn on one camera
	mx->sensors=TRUE;									// we want sensor data
	mx->dts=FALSE;										// dont want date time strings sent to us
	mx->imagesizes=TRUE;									// send MSG_STATS if max active camera image sizes change
	mx->server_settings=TRUE;								// send MSG_SERVERSETTINGS when settings change
	mx->comment=TRUE;									// text comment from server processes.
}



void reload_image(FL_OBJECT * obj, unsigned char *jpg_buffer, int jpg_size, int doscale)
{
	FL_IMAGE	*camimage = NULL;
	int ih=0, iw=0;
	int tout=0;

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

	if (camimage==NULL)
		camimage = flimage_alloc();

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
	if (doscale==TRUE)
 		flimage_scale(camimage, obj->w, obj->h, FLIMAGE_NOSUBPIXEL); 

// hack test, streth image to match window, it works, maybe use it?
//fl_get_winsize(fd_main->main->window,&iw,&ih);
//flimage_scale(camimage, iw-5, ih-70, FLIMAGE_NOSUBPIXEL); 

	flimage_sdisplay( camimage, FL_ObjWin( obj ) );

	flimage_free(camimage);
	camimage=NULL;
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



//JA
void update_serversettings_form()
{
	char st[512];
	fl_set_button(fd_serversettings->enable_archiver, se.enable_archiver);
	fl_set_button(fd_serversettings->exit_on_archive_error, se.exit_on_archive_error);
	fl_set_button(fd_serversettings->archive_verbose, se.archive_verbose);

	// if archiver is enabled then show green tick if it is operating ok or red x if not
	//if (se.enable_archiver!=TRUE)
		//fl_hide_object(fd_serversettings->archiving_ok);
	//else	fl_show_object(fd_serversettings->archiving_ok);

	if (ss.archiving_ok==TRUE)
	{
		fl_set_pixmap_file( fd_serversettings->archiving_ok, "green_tick.xpm" );
		fl_redraw_form( fd_serversettings->serversettings);
	}
	else	
	{
		fl_set_pixmap_file( fd_serversettings->archiving_ok, "red_x.xpm" );
		fl_redraw_form( fd_serversettings->serversettings);
	}

	if (se.archive_rate_ms==0)
		fl_set_slider_value(fd_serversettings->archive_rate_fps, 0);
	else	fl_set_slider_value(fd_serversettings->archive_rate_fps, 1000/se.archive_rate_ms);
	if (se.sarchive_rate_ms==0)
		fl_set_slider_value(fd_serversettings->sarchive_rate_fps, 0);
	else	fl_set_slider_value(fd_serversettings->sarchive_rate_fps, 1000/se.sarchive_rate_ms);

	sprintf(st,"%d ms",se.archive_rate_ms);
	fl_set_object_label(fd_serversettings->archive_rate_ms,st);
	sprintf(st,"%d ms",se.sarchive_rate_ms);
	fl_set_object_label(fd_serversettings->sarchive_rate_ms,st);

	fl_set_input(fd_serversettings->archive_path,se.archive_path);


	// Bottom half of form
	fl_set_button(fd_serversettings->docrc,se.docrc);
	fl_set_button(fd_serversettings->enable_http,se.enable_http);
	fl_set_button(fd_serversettings->enable_keyexchange,se.enable_keyexchange);

}

void ssapply_cb( FL_OBJECT * obj, long data )
{
	strcpy(se.archive_path, fl_get_input(fd_serversettings->archive_path));
	sendserversettings=TRUE;
	sleep_ms(100);
	fl_set_button(fd_serversettings->ssapply,FALSE);
}

void archive_rate_fps_cb( FL_OBJECT * obj, long data )
{
	se.archive_rate_ms = 1000 / (int)fl_get_slider_value(fd_serversettings->archive_rate_fps);
	//printf("se.archive_rate_ms =%d\n",se.archive_rate_ms); fflush(stdout);
	sendserversettings=TRUE;
	update_serversettings_form();
}

void sarchive_rate_fps_cb( FL_OBJECT * obj, long data )
{
	se.sarchive_rate_ms = 1000 / (int)fl_get_slider_value(fd_serversettings->sarchive_rate_fps);
	sendserversettings=TRUE;
	update_serversettings_form();
}

void archive_path_cb( FL_OBJECT * obj, long data )
{
}


void archive_image_size_cb( FL_OBJECT * obj, long data )
{
}

void enable_archiver_cb( FL_OBJECT * obj, long data )
{
	se.enable_archiver=fl_get_button(fd_serversettings->enable_archiver);
	sendserversettings=TRUE;
	update_serversettings_form();
}


void exit_on_archive_error_cb( FL_OBJECT * obj, long data )
{
	if (ss.archiving_ok!=TRUE)
	{
		fl_set_button(fd_serversettings->exit_on_archive_error, FALSE);
		se.exit_on_archive_error=FALSE;
		return;						// dont set if currently in error
	}
	se.exit_on_archive_error=fl_get_button(fd_serversettings->exit_on_archive_error);
	sendserversettings=TRUE;
}

void archive_verbose_cb( FL_OBJECT * obj, long data )
{
	se.archive_verbose=fl_get_button(fd_serversettings->archive_verbose);
	sendserversettings=TRUE;
}


void docrc_cb( FL_OBJECT * obj, long data )
{
	se.docrc=fl_get_button(fd_serversettings->docrc);
	sendserversettings=TRUE;
}

void enable_kx_cb( FL_OBJECT * obj, long data )
{
	se.enable_keyexchange=fl_get_button(fd_serversettings->enable_keyexchange);
	sendserversettings=TRUE;
}

void enable_http_cb( FL_OBJECT * obj, long data )
{
	se.enable_http=fl_get_button(fd_serversettings->enable_http);
	sendserversettings=TRUE;
}





char largeviewtext[128];
void update_camerasettings_form()
{
	char st[512];
	fl_set_object_label(fd_camerasettings->ipaddr,stngs.server_ipaddr);
	fl_set_button(fd_camerasettings->autoexposure,cs[selected_cam].autoexposure);
	fl_set_button(fd_camerasettings->autobrightness,cs[selected_cam].autobrightness);
	fl_set_button(fd_camerasettings->autocontrast,cs[selected_cam].autocontrast);

	cs[selected_cam].autoexposure= fl_get_button(fd_camerasettings->autoexposure);
	if (cs[selected_cam].autoexposure==TRUE)
		fl_hide_object(fd_camerasettings->exposure);
	else	fl_show_object(fd_camerasettings->exposure);
	cs[selected_cam].autobrightness= fl_get_button(fd_camerasettings->autobrightness);
	if (cs[selected_cam].autobrightness==TRUE)
		fl_hide_object(fd_camerasettings->brightness);
	else	fl_show_object(fd_camerasettings->brightness);
	cs[selected_cam].autocontrast= fl_get_button(fd_camerasettings->autocontrast);
	if (cs[selected_cam].autocontrast==TRUE)
		fl_hide_object(fd_camerasettings->contrast);
	else	fl_show_object(fd_camerasettings->contrast);


	if (cs[selected_cam].update_rate_ms==0)
		fl_set_slider_value(fd_camerasettings->update_rate_ms, 0);
	else	fl_set_slider_value(fd_camerasettings->update_rate_ms, 1000/cs[selected_cam].update_rate_ms);
	sprintf(st,"%d ms   = ",cs[selected_cam].update_rate_ms);
	fl_set_object_label(fd_camerasettings->update_rate_mstext,st);

	fl_set_slider_value(fd_camerasettings->quality, cs[selected_cam].quality);
	fl_set_slider_value(fd_camerasettings->brightness, cs[selected_cam].brightness);
	fl_set_slider_value(fd_camerasettings->exposure, cs[selected_cam].exposure);
	fl_set_slider_value(fd_camerasettings->contrast, cs[selected_cam].contrast);

	fl_set_button(fd_camerasettings->fliph, cs[selected_cam].fliph);
	fl_set_button(fd_camerasettings->flipv, cs[selected_cam].flipv);

	fl_set_button(fd_camerasettings->docaption, cs[selected_cam].docaption);
	if (cs[selected_cam].docaption==TRUE)
		fl_show_object(fd_camerasettings->captext);
	else	fl_hide_object(fd_camerasettings->captext);

	sprintf(st,"%d x %d",cs[selected_cam].width,cs[selected_cam].height);
	fl_set_object_label(fd_camerasettings->resolutiontext,st);

	fl_set_button(fd_camerasettings->dodatetime,cs[selected_cam].dodatetime);
	fl_set_button(fd_camerasettings->publicview,cs[selected_cam].publicview);

	fl_set_object_label(fd_camerasettings->largeviewtext,largeviewtext);

	fl_set_input(fd_camerasettings->captext,cs[selected_cam].captext);

	cs[selected_cam].docustomdate=fl_get_button(fd_camerasettings->docustomdate);
	if (cs[selected_cam].docustomdate==TRUE)
		fl_show_object(fd_camerasettings->customdate);
	else	fl_hide_object(fd_camerasettings->customdate);
	fl_set_input(fd_camerasettings->customdate, cs[selected_cam].customdate);

	//sprintf(st,"ID: %s  IP: %s",cs[selected_cam].id);
	sprintf(st,"ID: %s ",cs[selected_cam].id);
	fl_set_object_label(fd_camerasettings->id,st);


	// These two are really from server_settings, but present here for convenience
	fl_set_button(fd_camerasettings->samerescams, se.samerescams);
	fl_set_button(fd_camerasettings->samedatecams, se.samedatecams);
}


void changecam_cb( FL_OBJECT * obj, long data )
{
	char st[32];

	if (data==1)
		selected_cam--;
	if (data==2)
		selected_cam++;
	if (selected_cam<=0)
		selected_cam=1;
	if (selected_cam>JC_MAX_CAMS)
		selected_cam=JC_MAX_CAMS;
	sprintf(st,"CAM %02d",selected_cam);
	fl_set_object_label(fd_camerasettings->cameralabel,st);

	update_camerasettings_form();
	update_messagemask(&mm, selected_cam);							// mask to default for single camera
	mm.sensors=TRUE;									// we want sensor data
	jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);
	print_struct_camerasettings(&cs[selected_cam]);
	sent_reqid = jc_request_image(&ky, fdsock, 0, IFULL, selected_cam);			// Ask for an image
}

void quality_default_cb( FL_OBJECT * obj, long data )
{
	cs[selected_cam].quality=75;								// default quality
	fl_set_slider_value(fd_camerasettings->quality, cs[selected_cam].quality);
	sendcamsettings=TRUE;
	sleep_ms(100);
	fl_set_button(obj,0);
}



void customdate_cb( FL_OBJECT * obj, long data )
{
	strcpy(cs[selected_cam].customdate, fl_get_input(fd_camerasettings->customdate));
	update_camerasettings_form();
	sendcamsettings=TRUE;
}


void dodatetime_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].dodatetime=fl_get_button(fd_camerasettings->dodatetime);
	sendcamsettings=TRUE;
}

void ml_cb( FL_OBJECT * obj, long data )
{   
	sendcamsettings=TRUE;
}


void fps_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].update_rate_ms = 1000 / (int)fl_get_slider_value(fd_camerasettings->update_rate_ms);
	update_camerasettings_form();
	sendcamsettings=TRUE;
}


void fliph_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].fliph = fl_get_button(fd_camerasettings->fliph);
	sendcamsettings=TRUE;
}


void flipv_cb( FL_OBJECT * obj,  long data )
{   
	cs[selected_cam].flipv = fl_get_button(fd_camerasettings->flipv);
	sendcamsettings=TRUE;
}


void ircuttest_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].ircut= fl_get_button(fd_camerasettings->ircuttest);
	sendcamsettings=TRUE;
}


void publicview_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].publicview=fl_get_button(fd_camerasettings->publicview);
	sendcamsettings=TRUE;
}


void qual_cb( FL_OBJECT * obj, long data )
{   
	printf("qual_cb()\n"); fflush(stdout);
	cs[selected_cam].quality = (int)fl_get_slider_value(fd_camerasettings->quality);
	//update_camerasettings_form();
	sendcamsettings=TRUE;
}


void exp_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].exposure = (int)fl_get_slider_value(fd_camerasettings->exposure);
	sendcamsettings=TRUE;
}


void cs_cb( FL_OBJECT * obj, long data )
{   
	switch(data)
	{
		case 1:  
			cs[selected_cam].autoexposure = fl_get_button(fd_camerasettings->autoexposure);
		break;
		case 2:  
			cs[selected_cam].autobrightness = fl_get_button(fd_camerasettings->autobrightness);
		break;
		case 3:  
			cs[selected_cam].autocontrast = fl_get_button(fd_camerasettings->autocontrast);
		break;
	}

	update_camerasettings_form();
	sendcamsettings=TRUE;
}


void docaption_cb( FL_OBJECT * obj, long data )
{
	cs[selected_cam].docaption=fl_get_button(fd_camerasettings->docaption);
	update_camerasettings_form();
	sendcamsettings=TRUE;
}


void br_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].brightness = (int)fl_get_slider_value(fd_camerasettings->brightness);
	sendcamsettings=TRUE;
}

void co_cb( FL_OBJECT * obj, long data )
{   
	cs[selected_cam].contrast = (int)fl_get_slider_value(fd_camerasettings->contrast);
	sendcamsettings=TRUE;
}


void apply_cb( FL_OBJECT * obj, long data )
{   
	int w=0;
	int h=0;
	int i=0;

	w=atoi(fl_get_input(fd_camerasettings->iwidth));
	h=atoi(fl_get_input(fd_camerasettings->iheight));
	//printf("%dx%d\n",w,h); fflush(stdout);
	if ( (w>0) & (h>0) & (w<=JC_MAX_WIDTH) & (h<=JC_MAX_HEIGHT) )
	{
		cs[selected_cam].requested_width=w;
		cs[selected_cam].requested_height=h;
		sendcamsettings=TRUE;
	}

	if (se.samerescams==TRUE)								// Same res for all cams
	{
		for (i=1;i<JC_MAX_CAMS+1;i++)							// for all cams
		{
			if (i!=selected_cam)							// except the cam we are modifying
			{
				cs[i].requested_width=cs[selected_cam].requested_width;		// copy res to all
				cs[i].requested_height=cs[selected_cam].requested_height;
			}
			jc_msg_camerasettings(&ky, &cs[i], fdsock, 101);			// update the server with new camera settings
		}
	}
	sleep_ms(100);
	fl_set_button(obj,0);
}


void  clipscale_cb( FL_OBJECT * obj, long data )
{   
}


void  samedate_cb( FL_OBJECT * obj, long data )
{
	se.samedatecams=fl_get_button(fd_camerasettings->samedatecams);
	sendserversettings=TRUE;
}

void  sameres_cb( FL_OBJECT * obj, long data )
{
	se.samerescams=fl_get_button(fd_camerasettings->samerescams);
	sendserversettings=TRUE;
}


// Tabfolder
void tabs_cb( FL_OBJECT * obj, long data )
{
	current_form=fl_get_active_folder_number(obj);
	if (current_form != pcurrent_form)								// form changed
	{
		pcurrent_form = current_form;
		//force_request=TRUE;
	}


	// Just changed to this form ....
	switch (current_form)
	{
		case FORM_SERVERSETTINGS:
			jc_request_server_settings(fdsock, 0);						// ask for current settings
		break;

		case FORM_CAMERASETTINGS:
			force_request=TRUE;
		break;
	}
}





#define LCH		FL_WHITE
//#define LCU		FL_COL1
#define LCU		FL_BOTTOM_BCOL		// Dark grey
// Set label color to bright if camera is active, dim if not
void highlight_active_cams()
{
	int i;

	//printf(" highlight_active_cams()\n"); fflush(stdout);
	for (i=1;i<JC_MAX_CAMS+1;i++)
	{
   		//if (sen.sensor_active[JC_SENSTYPE_CAMERA][i]==TRUE)
			//fl_set_object_lcolor (fd_cameraview->bcam[i], LCH);
		//else 	fl_set_object_lcolor (fd_cameraview->bcam[i], LCU);
	}
}



#define HIGHLIGHT	FL_DARKGREEN
#define HB		FL_BLACK
void highlight_button( int b )
{
	if (verbose==TRUE)
		printf("%s(%05d): Changed to camera %d\n",PROGNAME,getpid(),b);

        selected_cam=b;
	update_messagemask(&mm, selected_cam);
	jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);
	sleep_ms(2);
	force_request=TRUE;
}





void jc_message_parser(int msgtype, uint16_t reqid, uint64_t sendtimems)
{
	int i=0;
	uint32_t  crc=0;
	uint32_t  ct=0;
	char st[1024];

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


		case JC_MSG_TIMEMS:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_TIMEMS\n",PROGNAME,getpid());
			psc_time_diff_ms=sc_time_diff_ms;
			sc_time_diff_ms=current_timems() - sendtimems;			// our local time - servers frame send time 
			printf("sc_time_diff_ms=%d\n",sc_time_diff_ms);
			fflush(stdout);
		break;


		case JC_MSG_STATS:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_STATS  reqid=%d\n",PROGNAME,getpid(),reqid);
			print_struct_statistics(msgst);
			// Server might not have seen any cameras yet, 0 values are possible
			//if ( (msgst->camera_largest_width_active>0) & (msgst->camera_largest_height_active>0) )
				//changesize(msgst->camera_largest_width_active, msgst->camera_largest_height_active);
		break;

		case JC_MSG_IMAGE:
			image_received_time=current_timems();
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_IMAGE  reqid=%d  img=%d  cam=%d\n",PROGNAME,getpid(),reqid,msgi->img,msgi->cam);

			if (valid_image_cam(msgi->img, msgi->cam, PROGNAME, "\0", FALSE))
				return;


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
			jc_image_copy(&shm->im, msgi);							// put in shared memory ready to jpeg decode

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
		break;


		case JC_MSG_SENS_ACTIVE:
			if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_ACTIVE  senstype=%d\n",PROGNAME,getpid(),msgs->senstype);
			if (msgs->senstype>=JC_MAX_SENSTYPE-1)
				return;
			for (i=0;i<=JC_MAX_SENSORS+1;i++)
			{
				sen.sensor_active[msgs->senstype][i] = msgs->sensor_active[i];
				//printf("i=%d   msgs->sensor_active[i]=%d\n",i,msgs->sensor_active[i]);
			}
			if (msgs->senstype==JC_SENSTYPE_CAMERA)
				highlight_active_cams();
		break;


		case JC_MSG_SENS_VALUES:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SENS_VALUES senstype=%d\n",PROGNAME,getpid(),msgv->senstype);

			if (msgv->senstype>=JC_MAX_SENSTYPE-1)
				return;
			for (i=0;i<JC_MAX_SENSORS+1;i++)
			{
				// copy into our local sensor state 
				sen.sensor_fvalue[msgv->senstype][i] = msgv->sensor_fvalue[i];
				if (sen.sensor_active[msgv->senstype][i]==TRUE)
					printf("S:%d\tA:%d\tV:%02.1f\n",i, sen.sensor_active[msgv->senstype][i], sen.sensor_fvalue[JC_SENSTYPE_TEMP][i]); 

				// Update sensor form with temp sens values
				if (sen.sensor_active[msgv->senstype][i]==TRUE)
				{
					sprintf(st,"T%d = %02.1f\n",i, sen.sensor_fvalue[JC_SENSTYPE_TEMP][i]);
					switch(i)
					{
						//case 1:   fl_set_object_label(fd_sensors->sen_t1, st); break;
						//case 2:   fl_set_object_label(fd_sensors->sen_t2, st); break;
						//case 3:   fl_set_object_label(fd_sensors->sen_t3, st); break;
						//case 4:   fl_set_object_label(fd_sensors->sen_t4, st); break;
						//case 5:   fl_set_object_label(fd_sensors->sen_t5, st); break;
						//case 6:   fl_set_object_label(fd_sensors->sen_t6, st); break;
					}
				}
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
		break;


		case JC_MSG_REGISTER:
		break;


		case JC_MSG_SERVERSETTINGS:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SERVERSETTINGS\n",PROGNAME,getpid());
			memcpy(&se,&msgbuffer,sizeof(struct jcamsys_server_settings));
		break;


		case JC_MSG_CAMERASETTINGS:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_CAMERASETTING cam=%d\n",PROGNAME,getpid(),msgcs->cam);
			if (reqid!=101)					// we get back the one we just sent, ignore it
			{
				memcpy(&cs[msgcs->cam],msgcs,sizeof(struct jcamsys_camerasettings));			// add to our local copy
				cs_changed[msgcs->cam]++;
				update_camerasettings_form();
				//print_struct_camerasettings(&cs[msgcs->cam]);
			}
		break;

		case JC_MSG_COMMENT:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_COMMENT server=%d cmt=%s\n",PROGNAME,getpid(),msgcm->server, msgcm->comment);
		break;


		case JC_MSG_SERVERSTATE:
			//if (debug==TRUE)
				printf("%s(%05d): got JC_MSG_SERVERSTATE \n",PROGNAME,getpid());
			memcpy(&ss,&msgbuffer,sizeof(struct jcamsys_serverstate));
		break;
	
	}
}




void process_form()
{
	unsigned int time_ms=0;
	int rms=0;
	int msgtype=0;
	int tdiff=0;
	uint16_t reqid=0;
	uint64_t sendtimems=0;
	int doscale=FALSE;
	//printf("current_form=%d\n",current_form);  fflush(stdout);

	time_ms=current_timems();
	rms=jcam_read_message(fdsock, &ky, (unsigned char*)&msgbuffer, sizeof(msgbuffer), &msgtype, &reqid, &sendtimems, 1);
	//if (rms!=0)
		//printf("rms=%d  msgtype=%d\n",rms,msgtype); fflush(stdout);
	if (rms==JC_HAVE_DATA)
		jc_message_parser(msgtype, reqid, sendtimems);					// fills in various structs and buffers

	if (rms==-1)
	{
		close(fdsock);
		fdsock=-1;
		state=0;
		printf("connection closed\n"); fflush(stdout);
		//fl_set_object_label(fd_cameraview->textbox,"Disconnected");
		//fl_show_object(fd_cameraview->textbox);
	}	

	// Do these regardless of which form is currently active
	if (rms==JC_HAVE_DATA)
	{
		switch (msgtype)
		{
			case JC_MSG_SERVERSETTINGS:
				update_serversettings_form();
				update_camerasettings_form();					// has two fields from server_settings
			break;

			case JC_MSG_COMMENT:
				if (msgcm->server==JC_CMNT_ARCHIVE)
				{
					stripnewline(msgcm->comment,sizeof(msgcm->comment));
					fl_set_object_label(fd_serversettings->comment,msgcm->comment);
				}
				printf("len=%d\n",strlen(msgcm->comment)); fflush(stdout);
			break;

			case JC_MSG_SERVERSTATE:
				update_serversettings_form();
			break;
		}
	}


	switch (current_form)
	{
		case FORM_SERVERSETTINGS:
		break;


		case FORM_CAMERASETTINGS:
			// Keep going if we loose an image
			tdiff=time_ms-image_received_time;									// time now  - time last image arrived
			if ( (tdiff > 1000) | (force_request==TRUE) )								// More than a second single last image ?
			{
				completed=TRUE;
				sent_reqid = jc_request_image(&ky, fdsock, 0, IFULL, selected_cam);
				last_image_request_time_ms=current_timems();
				image_received_time=time_ms;
				force_request=FALSE;
				printf("TIMEOUT\n"); fflush(stdout);
			}

			if (rms==JC_HAVE_DATA)
			{
				switch (msgtype)
				{
					case JC_MSG_FRAME_NUMBER:
						pframe_number[msgn->img][msgn->cam] = frame_number[msgn->img][msgn->cam];	// save previous value
						frame_number[msgn->img][msgn->cam] = msgn->frame_number;			// record new value
						if ( (msgn->cam == selected_cam) & (msgn->img==IFULL) )				// is it for our display ?
						{
							if (completed==TRUE)							// have we processed the last one yet?
							{
								sent_reqid = jc_request_image(&ky, fdsock, 0, IFULL, selected_cam);	// Ask for an image
								last_image_request_time_ms=current_timems();
								completed=FALSE;						// cycle starts again
							}
						}
						return;
					break;

					case JC_MSG_IMAGE:
						//printf("MSG_IMG reqid=%d   img=%d  cam=%d  %dx%d bytes=%d\n",
							//reqid,msgi->img,msgi->cam,msgi->width,msgi->height,msgi->cbytes); fflush(stdout);
						//print_struct_jcamsys_image(msgi);
						largeviewtext[0]=0;
						if (msgi->update_rate_ms>0)
							sprintf(largeviewtext,"%d x %d  (%dkB) %d FPS = %dkB/S",msgi->width,msgi->height,(msgi->cbytes/1024),
								1000/msgi->update_rate_ms,((1000/msgi->update_rate_ms)*msgi->cbytes)/1024);
						fl_set_object_label(fd_camerasettings->largeviewtext,largeviewtext);
						if (reqid==sent_reqid)								// is this the image we requested
						{
							completed=TRUE;								// then the cycle is complete
							if ( (msgi->cam==selected_cam) & (msgi->img == IFULL) )
							{
								doscale=fl_get_button(fd_camerasettings->clipscale);
								reload_image(fd_camerasettings->largeviewcanvas, (unsigned char*)msgi->cdata, msgi->cbytes, doscale);
								decoded_time=current_timems();
							}
						}
					break;
				}
				break;
			}
		break;



		//case FORM_SENSORS:
		//break;
	}
}



#define JC_CSTATE_INITIAL		0
#define JC_CSTATE_FIND_SERVER		1
#define JC_CSTATE_CONNECT_AND_AUTH	2
#define JC_CSTATE_RUNNING		3
#define JC_CSTATE_PAUSE_THEN_INITIAL	10


void jc_events()
{
	char st[2048];
	int i=0;
	int r=0;
	unsigned int time_ms=0;
	static int timer_initial_ms=0;
	static int timer_duration_ms=0;

	fl_check_forms();
	time_ms=current_timems();

	if ( ignore_framechange >0 )
		ignore_framechange--;

	if (shm->doexit==TRUE)
		dostop();

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
				//fl_set_object_label(fd_cameraview->textbox,"trying to YAFDP discover server");
				//fl_show_object(fd_cameraview->textbox);
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
						//fl_set_object_label(fd_cameraview->textbox,"YAFDP discover service busy");
						return;
                                        break;

					case -1:
						if (verbose==TRUE)
						printf("yafdp error of some sort\n");
						fflush(stdout);
						//fl_set_object_label(fd_cameraview->textbox,"YAFDP discover err");
						return;
                                        break;
				}
				if (strlen(stngs.server_ipaddr)<6)
				{
					//fl_set_object_label(fd_cameraview->textbox,"trying to YAFDP discover server - discover failed");
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
			//fl_set_object_label(fd_cameraview->textbox,st);
			//fl_redraw_object(fd_cameraview->textbox);
			printf("%s(%05d): connecting to server host=%s ip=%s port=%d\n",PROGNAME,getpid(),
                                stngs.server_hostname,stngs.server_ipaddr,stngs.server_tcp_port);
			fflush(stdout);
                        fdsock=jc_connect_to_server(stngs.server_ipaddr,stngs.server_tcp_port,FALSE);
			//printf("****** fdsock=%d\n",fdsock);
                        if (fdsock<0)
                        {
				fdsock=-1;
                                printf("%s(%05d): connect failed\n",PROGNAME,getpid());
				//fl_set_object_label(fd_cameraview->textbox,"connect failed");
				fl_check_forms();
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
                                i=jcam_authenticate_with_server(fdsock, (char*)&pw, pwlen, 5000);
				if (i==JC_OK)
				{
					printf("auth ok\n"); fflush(stdout);
					completed=TRUE;							// so we can send a request for image

					// Send mask first, so that when register happens we get all camera settings from server
					update_messagemask(&mm, selected_cam);
					jc_msg_messagemask(&ky, fdsock, 0, (struct jcamsys_messagemask*)&mm);

					registration.role=JC_ROLE_CONSOLE;
                                        jc_msg_register(&ky, &registration, fdsock);                    // tell server id&role

					jc_request_stats(fdsock, 0);					// ask for servers image sizes
					image_received_time=time_ms-100000;				// will force image fetch

					jc_request_server_settings(fdsock, 0);				// ask for current settings

					state=JC_CSTATE_RUNNING;
					//fl_hide_object(fd_cameraview->textbox);
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
			for (i=0;i<20;i++)
				process_form();

			if (slave_died==TRUE)
			{
				slave_died=FALSE;
				printf("%s(%05d): slave decoder %d died\n",PROGNAME,getpid(),cp);
				fflush(stdout);
				cp=start_slave_jpeg_decoder();
			}
		break;
	}
}




// Run once a second, or more frequently if called from other functions
void timer_cb (FL_OBJECT* obj, long data )
{
	int x=0;

	//printf("timerin\n");	fflush(stdout);

	if (sendcamsettings==TRUE)
	{
		jc_msg_camerasettings(&ky, &cs[selected_cam], fdsock, 101);				// special reqid = dont echo back
		sendcamsettings=FALSE;
	}

	if (sendserversettings==TRUE)
	{
		jc_msg_serversettings(&ky, &se, fdsock, 101);
		sendserversettings=FALSE;
printf("sent server settings\n"); fflush(stdout);
	}

	for (x=0;x<20;x++)
		jc_events();

	fl_set_timer(fd_main->tim, 0.001);
	//printf("timerout\n\n");	fflush(stdout);
}



void idle_callback()
{
	int x=0;
	//printf("idle\n");	fflush(stdout);

	for (x=0;x<20;x++)
		jc_events();
}

void camchange( FL_OBJECT* obj, long data )
{
	highlight_button(data);
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
	shm->doexit=TRUE;
	printf("cleanup time\n"); fflush(stdout);
	if (cp>0)
		kill(cp,SIGKILL);
	munmap(shm,sizeof(shm));
    	//fl_free( fd_cameraview );
    	fl_finish( );
	exit(0);
}


//FD_cameraview *fd_cameraview;
int main( int argc, char *argv[ ] )
{
	char cmstring[1024];
	int i=0;
	int l=0;
	int op_help=FALSE;
	int argnum=0;
	char st[2048];
	int smfd=-1;

	//image_received_time=current_timems()-2000;
	strcpy(binname,argv[0]);
	srand(time(NULL));
	client_settings_defaults(&stngs);
    	//bzero(&camserver_ipaddr,sizeof(camserver_ipaddr));
	bzero(&sen,sizeof(struct jcamsys_sensorstate));
	bzero(&se,sizeof(struct jcamsys_server_settings));
	//signal(SIGCHLD,slavedied);
	signal(SIGINT,dostop);

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

	//set_realtime();
	signal(SIGPIPE,SIG_IGN);
	printf("%s(%05d): %s version %s (c)2018 J.Andrews (jon@jonshouse.co.uk)\n",PROGNAME,getpid(),PROGNAME,VERSION);
	printf("%s(%05d): ",PROGNAME,getpid());
	printver();
	keylen=jcam_read_keyfile(stngs.filename_shared_key,&ky);
	if (keylen<0)
	{
		printf("shared key [%s] missing or bad, use jcmakekey\n",stngs.filename_shared_key);
		exit(1);
	}
	else	printf("%s(%05d): got valid key, length=%d\n",PROGNAME,getpid(),keylen);
	// Crypt plain text password into non plain text using key
	jcam_crypt_buf(&ky,(char*)&pw,sizeof(pw),pwlen);


	bzero(&registration,sizeof(registration));
	strcat(stngs.filename_client_registration,"_console");
	if (loadfile(stngs.filename_client_registration,(char*)&registration,sizeof(struct jcamsys_register))==JC_ERR)
		printf("%s(%05d): failed to load id, that is ok, the server should supply one\n",PROGNAME,getpid());

	smfd=-1;
	// Shared memory buffer between this processes and slave_jpeg_decoder
	shm = mmap(NULL, sizeof(struct sharedm), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, smfd, 0);
	cp=start_slave_jpeg_decoder();


	// Xforms init
    	fl_initialize( &argc, argv, 0, 0, 0 );

	fd_camerasettings	=	create_form_camerasettings();
    	fd_serversettings	= 	create_form_serversettings();
	fd_main			=	create_form_main();				// must be last


	// Hide control for now, maybe one day ....
	fl_hide_object(fd_serversettings->resolution);
	fl_hide_object(fd_serversettings->archive_image_size);
	fl_hide_object(fd_serversettings->archive_size);

	fl_set_button(fd_camerasettings->clipscale,1);			// turn it on for default
	fl_set_atclose((FL_FORM_ATCLOSE)killed_cb, (void *) fd_main->main);
	fl_show_form( fd_main->main, FL_PLACE_CENTERFREE, FL_FULLBORDER, PROGNAME );

	flimage_enable_jpeg();
    	highlight_button(selected_cam);
	fl_set_timer(fd_main->tim, 0.01);
	fl_set_idle_callback( (FL_APPEVENT_CB)idle_callback, NULL );
	sprintf(st,"%s version %s\n(c)2018 J.Andrews (jon@jonshouse.co.uk)\n",PROGNAME,VERSION);
    	fl_do_forms();

    	fl_finish( );
    	return 0;
}


