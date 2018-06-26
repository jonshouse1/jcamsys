// jcamsys_server

#define PROGNAME        "jc_server"
#define VERSION         "0.60"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_network.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_archiver.h"
#include "jcamsys_keyexchange.h"
#include "jcamsys_images.h"
#include "jcamsys_sensors_prot.h"
#include "id.h"

int verbose=FALSE;
int debug=FALSE;
int images_crypted=TRUE;
int quit=FALSE;
int feeder_cam=0;
int dontsendcs=FALSE;
int dontsendse=FALSE;
int save_cs=FALSE;
int save_ss=FALSE;


// Prototypes
int yafdpserver();
void jc_sensor();
void jc_httpd();


// Bug buffer, with different stucts overlayed over it
unsigned char msgbuffer[JC_MAX_MSGLEN];
struct jcamsys_header*		hdr  =(struct jcamsys_header*)&msgbuffer;

// requests, normally from clients
struct jcamsys_req_sens_info*   	reqn =(struct jcamsys_req_sens_info*)&msgbuffer;
struct jcamsys_req_sens_active* 	reqa =(struct jcamsys_req_sens_active*)&msgbuffer;
struct jcamsys_req_sens_values*  	reqv =(struct jcamsys_req_sens_values*)&msgbuffer;
struct jcamsys_req_image*       	reqi =(struct jcamsys_req_image*)&msgbuffer;
struct jcamsys_feeder_register*		regf =(struct jcamsys_feeder_register*)&msgbuffer;

// messages, normally from the server in response to requests
struct jcamsys_header*			msgh =(struct jcamsys_header*)&msgbuffer;
struct jcamsys_ack*			msga =(struct jcamsys_ack*)&msgbuffer;
struct jcamsys_image*			msgi =(struct jcamsys_image*)&msgbuffer;
struct jcamsys_sens_info*   		msgf =(struct jcamsys_sens_info*)&msgbuffer;
struct jcamsys_sens_active* 		msgs =(struct jcamsys_sens_active*)&msgbuffer;
struct jcamsys_sens_values*  		msgv =(struct jcamsys_sens_values*)&msgbuffer;
struct jcamsys_sens_value*  		msgav=(struct jcamsys_sens_value*)&msgbuffer;
struct jcamsys_frame_number*        	msgn =(struct jcamsys_frame_number*)&msgbuffer;
struct jcamsys_messagemask*		msgm =(struct jcamsys_messagemask*)&msgbuffer;
struct jcamsys_register*		msgr =(struct jcamsys_register*)&msgbuffer;
struct jcamsys_camerasettings*		msgcs=(struct jcamsys_camerasettings*)&msgbuffer;




// Globals
struct jcamsys_sensorstate sest;
struct jcamsys_messagemask mm;
struct sharedmem* jcsm = NULL;

struct jcamsys_key              ky;
struct jcamsys_sensorstate      sen;
struct jcamsys_image		csi;				// scratch buffer, copy out of shared mem into here then do write()
struct jcamsys_serverstate	pss;




int keylen=0;
int num_active_processes=1;
int sockaddrlen=sizeof(struct sockaddr_in);


// defaults
void clear_messagemask(struct jcamsys_messagemask* mx)
{
	int i=0;
	for (i=0;i<JC_MAX_CAMS+1;i++)
	{
		mx->frame_changes[IFULL][i]=FALSE;				// for each image size opt in or out of frame number messages
		mx->camera_settings[i]=FALSE;
	}
	mx->sensors=TRUE;
	mx->imagesizes=FALSE;
	mx->server_settings=FALSE;
	mx->comment=FALSE;
}



// Every time you close a socket another child dies
void child_died()
{
	//pid_t	p;
	int status;

	//p=wait(&status);
	wait(&status);
	num_active_processes--;
}




// Look at the jcsm->ci[X] array for an entry where id=NULL
int jc_camera_find_free_slot()
{
	int i=0;

	for (i=1;i<=JC_MAX_CAMS+1;i++)
	{
		if (jcsm->cs[i].id[0]==0)
			return(i);
	}
	return(JC_ERR);
}


// Look for this ID in the camerasettings, return the slot (camera number)
int jc_camera_find_slot(char* id)
{
	int i=0;

	for (i=1;i<=JC_MAX_CAMS+1;i++)						// find ID in current list of cameras
	{
		//printf("%s = %s?\n",jcsm->cs[i].id,id); fflush(stdout);
		if (strcmp(jcsm->cs[i].id,id)==0)
			return(i);
	}

	return(jc_camera_find_free_slot());					// if not found then pick next free slot
}



// TODO: move some of this to jcamsys_statistics.c maybe ?
void jc_clear_stats(struct jcamsys_statistics* st)
{
	st->archive_files_written=0;
	st->archive_file_write_errors=0;
	st->camera_largest_width_active=0;
	st->camera_largest_width_configured=0;
	st->camera_largest_height_configured=0;
}



// TODO: This code is wrong, largest size set but never reduced, maybe the large image camera has gone offline

void jc_update_stats(int sockfd, int action)					// something changed, keep stats up to date
{
	int c=0;
	int sc=FALSE;

	switch(action)
	{
		case JC_AC_REGISTER:
		case JC_AC_IMAGE:
			for (c=1;c<JC_MAX_CAMS+1;c++)
			{
				if (jcsm->senss.sensor_active[JC_SENSTYPE_CAMERA][c]==TRUE)	// if camera is active
				{
					if (jcsm->cs[c].width > jcsm->st.camera_largest_width_active)
					{
						jcsm->st.camera_largest_width_active=jcsm->cs[c].width;
						sc=TRUE;
					}
					if (jcsm->cs[c].height > jcsm->st.camera_largest_height_active)
					{
						jcsm->st.camera_largest_height_active=jcsm->cs[c].height;
						sc=TRUE;
					}
				}
			}
			if ( (sc==TRUE) &  (mm.imagesizes==TRUE) )			// if image sizes changed and client asked to be notified
				jc_msg_stats(&ky, &jcsm->st, sockfd, 0);
		break;
	}
}




// Populate image structure with default values
void jc_init_image(int cam)
{
	int i=0;
	int w=0;
	int h=0;

	for (i=0;i<JC_MAX_IMAGES;i++)
	{
		w=640; h=480;
		if (jcsm->se.samerescams==TRUE)						// All cameras same resolution ?
		{
			if (jcsm->cs[1].requested_width>640)
				w=jcsm->cs[1].requested_width;				// then take width and height of testcard from camera configuration
			if (jcsm->cs[1].requested_height>480)
				h=jcsm->cs[1].requested_height;
		}
		jc_image_prepare(&ky, &jcsm->ci[i][cam], i, cam, JC_IMAGE_TYPE_JPEG, w , h, 0, "", 0);
	}
}



//JA

// Put a jpeg holding image into camera N position
void jc_testcard(int cam)
{
	int i=0;
	int cbytes=0;
	unsigned char rgbdata[JC_MAX_WIDTH * JC_MAX_HEIGHT * 3];

	jc_init_image(cam);
	// Produce a testcard for each image size, jcsm->ci should have already been populated with sizes
	for (i=0;i<JC_MAX_IMAGES;i++)
	{
		jc_RGBsolidcolour((unsigned char*)&rgbdata, (jcsm->ci[i][cam].width * jcsm->ci[i][cam].height * 3), 0x00, 0x00, 0xff);
		cbytes = encode_jpeg_tomem((unsigned char*)&rgbdata, (unsigned char*)&jcsm->ci[i][cam].cdata, jcsm->ci[i][cam].width, jcsm->ci[i][cam].height, 75);
		jcsm->ci[i][cam].cbytes = cbytes;
		jcsm->ci[i][cam].img=i;
		jcsm->ci[i][cam].cam=cam;
		jcsm->ci[i][cam].update_rate_ms=1000;
		jcsm->ci[i][cam].timestamp = current_timems();
	}
	//print_struct_jcamsys_image(&jcsm->ci[0][cam]);
}








void deregister_cam(int cam)
{
	//printf("\n\ndereg %d\n",cam); fflush(stdout);
	jc_sensor_mark_inactive(&jcsm->senss, JC_SENSTYPE_CAMERA, cam);
	jc_testcard(cam);
}




// checks one camera per call
void timeout_cameras()
{
	unsigned long ct=0;
	unsigned long int tdiff=0;
	static int c=1;


	if (jcsm->senss.sensor_active[JC_SENSTYPE_CAMERA][c]==TRUE)
	{
		ct=current_timems();
		tdiff = ct - jcsm->ci[0][c].timestamp;
		//printf("cam %d tdiff=%d\n",c,tdiff); fflush(stdout);

		if (tdiff > 5000)
			deregister_cam(c);
	}
	c++;
	if (c>JC_MAX_CAMS+1)
		c=1;
}




// Multiple processes are working on this state, rather complex messages ques lets just poll it to see what changed
void poll_sensor_change(struct jcamsys_sensorstate* ss, int sockfd, int clear_sl)
{
	static struct jcamsys_sensorstate	sl;					// local copy
	int i=0;

	if (clear_sl==TRUE)
	{
		bzero(&sl,sizeof(struct jcamsys_sensorstate));
		return;
	}

	if (ss->initialised != TRUE)
		return;									// dont poll it until it is active

	// Check if 'what sensors are active' or sensor value has changed for each sensor type
	// send a message to keep clients up to date
	for (i=0;i<JC_MAX_SENSTYPE;i++)
	{
		if (ss->sensor_active_changed[i] != sl.sensor_active_changed[i])
		{
			sl.sensor_active_changed[i]=ss->sensor_active_changed[i];
			jc_msg_sens_active(&ky, ss, sockfd, 0, i);
		}
		if (ss->sensor_value_changed[i] != sl.sensor_value_changed[i])
		{
			sl.sensor_value_changed[i]=ss->sensor_value_changed[i];
			jc_msg_sens_values(&ky, ss, sockfd, 0, i);
		}
	}
}






// As the server forks each instance handles one one host. We store the details the client sent
// us when it registered upon connection
struct jcamsys_register		client_registration;
int pcs_changed[JC_MAX_CAMS+1];
int pse_changed;
int parchive_comment_changed;

void jc_server(int sockfd, char*ipaddr)
{
	uint16_t	pframe_number[JC_MAX_IMAGES][JC_MAX_CAMS+1];
	int rms=0;	// read message status
	int i=0;
	int c=0;
	int r=0;
	int msgtype;
	int sec=0;
	static int psec=0;
	char dts[32];
	uint32_t crc=0;
	uint16_t reqid=0;
	uint64_t sendtimems=0;
	struct tm* tm_info;
	time_t timer;

	if (jcsm->quit==TRUE)											// if a child shuts us down
	{
        	signal(SIGCHLD, SIG_DFL);
        	signal(SIGPIPE, SIG_DFL);
		quit=TRUE;
	}

	// send messages to client about cameras that have changed
	sec=get_clockseconds();											// just started new second
	if (sec!=psec)
	{
		psec=sec;
		if (mm.dts==TRUE)
		{
			if (jcsm->cs[feeder_cam].docustomdate==TRUE)						// this camera has a custom date/time
			{
				time(&timer);
				tm_info = localtime(&timer);
				strftime(dts,sizeof(dts),jcsm->cs[feeder_cam].customdate,tm_info);	
			}
			else datetime(&dts,FALSE);								// use default date/time string
			jc_msg_dts(&ky, sockfd, (char*)&dts);
		}

		if (save_cs==TRUE)
		{
			save_cs=FALSE;										// use a flag to ensure we save at most one file a second
			r=savefile("jcamsys/jcamsys_camerasettings.dat",(char*)jcsm->cs,sizeof(jcsm->cs),0666);
			//printf("saved camerasettings.dat %d bytes\n",r); fflush(stdout);
			if (r!=sizeof(jcsm->cs))
			{
				fprintf(stderr,"%s(%05d): Error saving jcamsys/jcamsys_camerasettings.dat (%s)\n",PROGNAME,getpid(),strerror(errno));
				fflush(stderr);
			}
		}

		if (save_ss==TRUE)
		{
			save_ss=FALSE;
			r=savefile("jcamsys/jcamsys_serversettings.dat",(char*)&jcsm->se,sizeof(jcsm->se),0666);
			printf("saved serversettings.dat %d bytes\n",r); fflush(stdout);
			if (r!=sizeof(jcsm->se))
			{
				fprintf(stderr,"%s(%05d): Error saving jcamsys/jcamsys_serversettings.dat r=%d(%s)\n",PROGNAME,getpid(),r,strerror(errno));
				fflush(stderr);
			}
		}
	}


	if (feeder_cam<=0)
	{
		for (i=0;i<JC_MAX_CAMS;i++)
		{
			if ( jcsm->ci[IFULL][i].frame_number != pframe_number[IFULL][i] )
			{
				pframe_number[IFULL][i] = jcsm->ci[IFULL][i].frame_number;
				if ( mm.frame_changes[IFULL][i]==TRUE )
					jc_msg_frame_number (&ky, jcsm->ci[IFULL][i].frame_number, sockfd, IFULL, i);
			}
		}
	}
	if (mm.sensors==TRUE)
		poll_sensor_change((struct jcamsys_sensorstate*)&jcsm->senss, sockfd, FALSE);

	for (i=1;i<JC_MAX_CAMS+1;i++)
	{
		if (jcsm->cs_changed[i]!=pcs_changed[i])
		{
			pcs_changed[i]=jcsm->cs_changed[i];

			if (mm.camera_settings[i]==TRUE)							// client has asked for camera settings changes
			{
				if (dontsendcs==TRUE)								// flag, dont send camera settings, except to camera
					dontsendcs=FALSE;							// flag is a one time deal
				else	jc_msg_camerasettings (&ky, &jcsm->cs[i], sockfd, 0);
			}
			if (feeder_cam==i)
				jc_msg_camerasettings (&ky, &jcsm->cs[i], sockfd, 0);				// If this is the camera I am servicing
		}
	}

	if (jcsm->se_changed != pse_changed)
	{
		pse_changed=jcsm->se_changed;

		if (mm.server_settings==TRUE)
		{
			if (dontsendse==TRUE)
				dontsendse=FALSE;
			else	jc_msg_serversettings(&ky, &jcsm->se, sockfd, 0);
		}
	}


	if (jcsm->archive_comment_changed != parchive_comment_changed)
	{
		parchive_comment_changed=jcsm->archive_comment_changed;
		if (mm.comment==TRUE)
		{
			jc_msg_comment(&ky, JC_CMNT_ARCHIVE, (char*)&jcsm->archive_comment, sockfd, 0);
		}
	}
	timeout_cameras();

	// if server state changed
	if ( (jcsm->ss.archiving_ok != pss.archiving_ok) | (jcsm->ss.kx_active != pss.kx_active) | (jcsm->ss.http_active != pss.http_active) )
	{
		memcpy(&pss,&jcsm->ss,sizeof(struct jcamsys_serverstate));
		jc_msg_serverstate(&ky, &jcsm->ss, sockfd, 0);
printf("sent serverstate\n"); fflush(stdout);
	}


	rms=jcam_read_message(sockfd, &ky, (unsigned char*)&msgbuffer, sizeof(msgbuffer), &msgtype, &reqid, &sendtimems, 1);
	if (rms==JC_HAVE_DATA)
	{
		//printf("have data, rms=%d  msgtype=%d\n",rms,msgtype);
		switch (msgtype)							// what type of message ?
		{
			// Decode REQuests, send MSGs back
			case JC_REQ_TIMEMS:						// client asked for tiny packet, time is in header
				jc_msg_timems(&ky, sockfd,reqid);
				jc_update_stats(sockfd, JC_AC_NONE);
			break;

			case JC_REQ_STATS:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_STATS\n",PROGNAME,getpid());
				jc_msg_stats(&ky, &jcsm->st, sockfd, 0);
			break;

			case JC_REQ_SENS_ACTIVE:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_SENS_ACTIVE\n",PROGNAME,getpid());
				jc_msg_sens_active(&ky, (struct jcamsys_sensorstate*)&jcsm->senss, sockfd, reqid, reqa->senstype);
				jc_update_stats(sockfd, JC_AC_NONE);
			break;


			case JC_REQ_SENS_VALUES:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_SENS_VALUES\n",PROGNAME,getpid());
				jc_msg_sens_values(&ky, (struct jcamsys_sensorstate*)&jcsm->senss, sockfd, 0, reqv->senstype);
				jc_update_stats(sockfd, JC_AC_NONE);
			break;


			case JC_REQ_SENS_EVERYTHING:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_EVERYTHING\n",PROGNAME,getpid());
				jc_update_stats(sockfd, JC_AC_NONE);
			break;


			case JC_REQ_IMAGE:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_IMAGE reqid=%d img=%d cam=%d\n",PROGNAME,getpid(),
					       reqid,reqi->img,reqi->cam);
				if (valid_image_cam(reqi->img,reqi->cam,PROGNAME,ipaddr,FALSE)<0)
					return;
				//printf("sending this now..\n");
				//print_struct_jcamsys_image( &jcsm->ci[reqi->img][reqi->cam] );
				// The struct cdata is large, copy the number of bytes in the image plus enough for rest of the struct members
				jc_sm_get_cdata_lock(jcsm, getpid(), reqi->img, reqi->cam);			// grab lock
				jc_image_copy( &csi, &jcsm->ci[reqi->img][reqi->cam]);				// copy image from shared mem
				jc_sm_release_cdata_lock(jcsm, getpid(), reqi->img, reqi->cam);			// release the lock
				csi.img=reqi->img;
				csi.cam=reqi->cam;
				jc_msg_image(&ky, &csi, sockfd, jcsm->se.docrc, reqid);				// send the image we copied
				jc_update_stats(sockfd, JC_AC_REQ_IMAGE);
			break;


			case JC_REQ_CAMERASETTINGS:
			break;

			case JC_REQ_SERVERSETTINGS:
				if (debug==TRUE)
					printf("%s(%05d): got JC_REQ_SERVERSETTINGS reqid=%d\n",PROGNAME,getpid(),reqid);
				jc_msg_serversettings(&ky, &jcsm->se, sockfd, reqid);
				//jc_update_stats(sockfd, JC_AC_SERVERSETTINGS);
			break;


			case JC_MSG_MESSAGEMASK:
				if (debug==TRUE)
					printf("%s(%05d): got JC_MESSAGEMASK\n",PROGNAME,getpid());
				memcpy(&mm,msgm,sizeof(struct jcamsys_messagemask));
				printf("%s(%05d): messagemask, ",PROGNAME,getpid());
				printmm(&mm);
				printf(" sensors=%d  dts=%d  imagesizes=%d serversettings=%d comment=%d\n",
					mm.sensors, mm.dts, mm.imagesizes,mm.server_settings,mm.comment);
				fflush(stdout);
				jc_update_stats(sockfd, JC_AC_MESSAGEMASK);
			break;



			case JC_MSG_CAMERASETTINGS:
				//if (debug==TRUE)
					printf("%s(%05d): got JC_MSG_CAMERASETTINGS, cam=%d\n",PROGNAME,getpid(),msgcs->cam);
				//print_struct_camerasettings((struct jcamsys_camerasettings*)&msgbuffer); 
				memcpy((char*)&jcsm->cs[msgcs->cam],msgcs,sizeof(struct jcamsys_camerasettings));	// Copy new settings into shared mem
				save_cs=TRUE;									// defer saving for a while
				jc_update_stats(sockfd, JC_AC_CAMERASETTINGS);
				if (reqid==101)									// special mark, please dont send it back
					dontsendcs=TRUE;							// stop main loop from sending back to console
				jcsm->cs_changed[msgcs->cam]++;							// note camera setting changed

				// Regenerate testcard in new image size
				if (jcsm->se.samerescams==TRUE)							// same resolution for all cameras ?
				{
					for (i=0;i<JC_MAX_IMAGES;i++)
					{
						for (c=1;c<JC_MAX_CAMS;c++)
						{
							jcsm->ci[i][c].width=msgcs->requested_width;
							jcsm->ci[i][c].height=msgcs->requested_height;
						}
					}
				}
				for (c=1;c<JC_MAX_CAMS;c++)
					jc_testcard(c);
			break;



			case JC_MSG_SERVERSETTINGS:
				//if (debug==TRUE)
					printf("%s(%05d): got JC_MSG_SERVERSETTINGS\n",PROGNAME,getpid());
				memcpy((char*)&jcsm->se,&msgbuffer,sizeof(struct jcamsys_server_settings));
				jc_update_stats(sockfd, JC_AC_SERVERSETTINGS);
				//printf("reqid=%d\n",reqid); fflush(stdout);
				if (reqid==101)									// special mark, please dont send it back
					dontsendse=TRUE;							// stop main loop from sending back to console
				jcsm->se_changed++;
				save_ss=TRUE;									// at some point save the server settings
			break;



			case JC_MSG_IMAGE:
				if (debug==TRUE)
					printf("%s(%05d): got JC_MSG_IMAGE img=%d cam=%d\n",PROGNAME,getpid(),msgi->img,msgi->cam);

				if (valid_image_cam(msgi->img,msgi->cam,PROGNAME,ipaddr,FALSE)<0)
					return;

				msgi->timestamp = current_timems();						// always when server receives
				jc_sm_get_cdata_lock(jcsm, getpid(), msgi->img, msgi->cam);			// grab lock
				jc_image_copy( &jcsm->ci[msgi->img][msgi->cam], msgi);				// copy image to shared mem
				strcpy((char*)&jcsm->ci[msgi->img][msgi->cam].sipaddr,ipaddr);			// note its source IP (originating)
				jc_sm_release_cdata_lock(jcsm, getpid(), msgi->img, msgi->cam);			// release the lock

				if (jcsm->se.docrc==TRUE)
				{
					if (msgi->optional_crc32!=0)
					{
						jc_image_force_uncrypted(&ky,msgi);				// need to uncrypt to check CRC
						crc=rc_crc32(0, (char*)&msgi->cdata, msgi->cbytes);
						if (crc != msgi->optional_crc32)
						{
							printf("FAIL IMG:%1d CAM:%02d FN:%06d CRC:%08X OURCRC:%08X Bytes:%d LB:%02X\n",
								msgi->img, msgi->cam, msgi->frame_number,msgi->optional_crc32,crc,
								msgi->cbytes,(unsigned char)msgi->cdata[msgi->cbytes]); 
							fflush(stdout);
							return;
						}
					}
				}
				//print_struct_jcamsys_image(msgi);
				jc_sensor_mark_active(&jcsm->senss, JC_SENSTYPE_CAMERA, msgi->cam);		// Mark this camera as active
				jc_archiver_add_image(msgi);							// always offer image to archiver
				jc_update_stats(sockfd, JC_AC_IMAGE);
			break;




//struct __attribute__((packed)) jcamsys_sens_value
//{
        //uint16_t                senstype;
        //uint16_t                sensor;
        //float                   sensor_fvalue;
        //uint16_t                sensor_ivalue;
        //char                    sensor_cvalue;
//};
//msgav	  = a value


			//case JC_MSG_SENS_VALUE:
				//if (debug==TRUE)
					printf("%s(%05d): got JC_MSG_SENS_VALUE\n",PROGNAME,getpid());
				//jc_sensor_value(

				//jc_update_stats(sockfd, JC_AC_SENS);
			//break;



			// register with server, for cameras camera information will be sent back
			case JC_MSG_REGISTER:
				//if (debug==TRUE)
					//printf("%s(%05d): got JC_MSG_REGISTER\n",PROGNAME,getpid());

				memcpy ( &client_registration, msgr, sizeof(struct jcamsys_register));
				// If the client registration ID is blank then we need to give it a unique ID.
				if (strlen(client_registration.id)==0)
				{
					random_md5((char*)&client_registration.id);
					jc_msg_register(&ky, &client_registration, sockfd);
				}

				//printf("id=%s\n",client_registration.id); 
				//printf("role=%d\n",client_registration.role);

				// Do registration
				switch (client_registration.role)
				{
					// feeder_cam is the 'camera number' for this cameras session.
					// the service can remember user settings, tying the camera number to a specific camera (by id)
					// if the camera does not have any stored settings then the next free (non comitted) camera number
					// is used.
					case JC_ROLE_CAMERA:
						feeder_cam = jc_camera_find_slot(client_registration.id);
						printf("%s(%05d): CAM%d ID:%s\n",PROGNAME,getpid(),feeder_cam,client_registration.id); 
						fflush(stdout);
						if (feeder_cam>0)
						{
							// copy the id from the client registration request into the camera slot
							// the presence of the id shows that this slot is now taken
							strcpy( jcsm->cs[feeder_cam].id, client_registration.id );

							// Now send camera settings to the camera itself, with luck it should start sending images
							jc_msg_camerasettings (&ky, &jcsm->cs[feeder_cam], sockfd,0);
							//print_struct_camerasettings( &jcsm->cs[feeder_cam]);
							//printf("%s(%05d): sent registration for camera %d to %s\n",PROGNAME,getpid(),feeder_cam,
								//client_registration.id);
							printf("%s(%05d): sent registration for camera %d to %s\n",PROGNAME,getpid(),feeder_cam,
								ipaddr);
							fflush(stdout);

							// Save entire settings structure to disk
							r=savefile("jcamsys/jcamsys_camerasettings.dat",(char*)jcsm->cs,sizeof(jcsm->cs),0666);
							if (r<=0)
							{
								printf("Error, failed to create/overwrite jcamsys/jcamsys_camerasettings.dat\n");
								exit(1);
							}
						}
						else
						{
							printf("%s(%05d): JC_MSG_REGISTER, failed to find a free camera slot, increase JC_MAX_CAMS?\n",PROGNAME,getpid());
							fflush(stdout);
							//quit=TRUE;
							return;
						}
					break;


					case JC_ROLE_VIEWER:
					break;


					case JC_ROLE_CONSOLE:
						printf("%s(%05d): CONSOLE ID:%s\n",PROGNAME,getpid(),client_registration.id); 
						fflush(stdout);
						// Send all camera settings
						for (i=1;i<JC_MAX_CAMS+1;i++)
							jc_msg_camerasettings (&ky, &jcsm->cs[i], sockfd, 0);
					break;
				}
				fflush(stdout);
				jc_update_stats(sockfd, JC_AC_REGISTER);
			break;
		}
	}

	if (rms==-1)									// socket closed?
	{
		quit=TRUE;
		return;
	}
	usleep(1000);
}


void socket_died()
{
	quit=TRUE;
	//printf("scoket died\n"); fflush(stdout);
}


int main(int argc, char **argv)
{
	int argnum;
        char cmstring[1024];
        int op_help=FALSE;
	char st[2048];
	int i=0;
	int r=0;
	int c=0;
	pid_t pidyafdp_listener;
	pid_t pidchild;
	int fd_serversock=-1;
	int client_sock=-1;
	struct sockaddr_in client;
	char ipaddr[16];

	// Sanity check config
	if (JC_MAX_SENSORS<JC_MAX_CAMS)
	{
		printf("error in jcamsys.h  JC_MAX_SENSORS must be >= JC_MAX_CAMS\n");
		exit(1);
	}


	// Memory shared between this process and its children
	jcsm  = mmap(NULL, sizeof(struct sharedmem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	bzero(jcsm,sizeof(struct sharedmem));
	bzero(&client_registration, sizeof(struct jcamsys_register));
	jc_clear_stats(&jcsm->st);

	// Initialise shared memory with sensible values
	// Set global size/fps
	//jcsm->gmode = JC_MODE_640x480;
	//jcsm->gwidth  = 0;
	//jcsm->gheight = 0;
	//jcsm->gfps    = 0;
	//jcsm->gquality= 0;
	//jcsm->gdocrc=TRUE;


        srand(time(NULL));
	settings_defaults(&jcsm->se);
	signal(SIGCHLD, child_died);
	signal(SIGPIPE, socket_died);

	if (mkdir("jcamsys",0777)==0)
		printf("%s(%05d): 'jcamsys' directory was missing, created it\n",PROGNAME,getpid());

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
                printf("%s (%s) <Options>\n",PROGNAME,VERSION);
                printf(" -p\t\tTCP port, default=%d\n",jcsm->se.server_tcp_port);
		printf(" -httpp\t\thttp server TCP port, default=%d\n",jcsm->se.http_server_tcp_port);
		printf(" -kxp\t\tKey exchange TCP port, default=%d\n",jcsm->se.kx_server_tcp_port); 
		printf(" -v\t\tVerbose - show messages\n");
		printf(" -debug\t\tDebug output useful for testing\n");
                printf(" -D\t\trun as Daemon\n");
		printf(" -noar\t\tDont archive\n");
		printf("\n Items disabled here can not be re-enabled by administration tools\n");
		printf(" -nodiscover\tDisable discovery service\n");
		printf(" -nokx\t\tDisable key exchange (kx) service\n");
		printf(" -nohttp\tDisable http server\n");
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
		jcsm->se.server_tcp_port=i;

	strcpy(cmstring,"-nodiscover");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		jcsm->se.nodiscover=TRUE;

	strcpy(cmstring,"-nokx");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		jcsm->se.nokx=TRUE;

	strcpy(cmstring,"-noar");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		jcsm->se.noar=TRUE;

	strcpy(cmstring,"-nohttp");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		jcsm->se.nohttp=TRUE;

	strcpy(cmstring,"-v");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		verbose=TRUE;

	strcpy(cmstring,"-debug");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		verbose=TRUE;
		debug=TRUE;
	}


	// Make sure we have password file and a valid encryption key before starting
	if (file_exists(jcsm->se.filename_passwd)!=TRUE)
	{
		printf("Password file [%s] missing, use jcam_mkpasswd\n",jcsm->se.filename_passwd);
		exit(1);
	}


	// Initialise 
	set_realtime();
	bzero((struct jcamsys_sensorstate*)&jcsm->senss,sizeof(struct jcamsys_sensorstate));
	jcsm->senss.initialised=TRUE;
	jc_sm_clearlocks(jcsm);



	printf("%s(%05d): jcamserver version %s (c)2018 Jonathan Andrews (jon@jonshouse.co.uk)\n",PROGNAME,getpid(),VERSION);
	if (images_crypted!=TRUE)
		printf("%s(%05d): !Disabling image encryption,  not recommended\n",PROGNAME,getpid());

	keylen=jcam_read_keyfile(jcsm->se.filename_shared_key,&ky);
	if (keylen<0)
	{
		printf("Shared key [%s] missing or bad, use jcmakekey\n",jcsm->se.filename_shared_key);
		exit(1);
	}
	else    printf("%s(%05d): got valid key, length=%d\n",PROGNAME,getpid(),keylen);
	if (getuid()==0)
		printf("%s(%05d): running UID=0 (root), this process can run with user privileges instead, see README\n",PROGNAME,getpid());
	fflush(stdout);


	// Make sure we are the only server of this type with discovery enabled
	if (jcsm->se.nodiscover!=TRUE)							// using discovery, their can be only one server
	{
		jc_discover_server((char*)&st,&i,FALSE, FALSE);
		if (strlen(st)>=6)
		{
			printf("Error, found another instance of 'jcamsysserver' ip=%s port=%d\n",st,i);
			exit(1);
		}
		else
		{
        		if (proc_find("yafdpd")<=0)                                                             // not running yafdpd ?
        		{
                		pidyafdp_listener=fork();
                		if (pidyafdp_listener==0)                                                       // I am the child
                		{
                        		printf("%s(%05d): started a yafdp server\n",PROGNAME,getpid());
                        		fflush(stdout);
                        		yafdpserver();
                        		exit(0);
				}
                	}
        	}
	}

	// For all images specify what sizes they should be, this work will be partially overwritten by jcamsys_camerasettings.dat
	for (i=0;i<JC_MAX_IMAGES;i++)
	{
		// for this camera what image sizes
		for (c=1;c<JC_MAX_CAMS+1;c++)
			jc_init_image(c);
	}

	for (i=1;i<JC_MAX_CAMS+1;i++)
		jc_camera_default(&jcsm->cs[i],i);


	// Try and load server settings, if we dont have one yet then create one with defaults
	r=loadfile("jcamsys/jcamsys_serversettings.dat",(char*)&jcsm->se,sizeof(jcsm->se));
	if (r==sizeof(jcsm->se))
		printf("%s(%05d): loaded jcamsys/jcamsys_serversettings.dat\n",PROGNAME,getpid());
	else
	{
		fprintf(stderr,"%s(%05d): Error loading jcamsys/jcamsys_sercersettings.dat r=%d (%s)\n",PROGNAME,getpid(),r,strerror(errno));
		r=savefile("jcamsys/jcamsys_serversettings.dat",(char*)&jcsm->se,sizeof(jcsm->se),0666);
		if (r!=sizeof(jcsm->se))
		{
			fprintf(stderr,"%s(%05d): Error saving jcamsys/jcamsys_serversettings.dat r=%d(%s)\n",PROGNAME,getpid(),r,strerror(errno));
			fflush(stderr);
		}
	}


	r=loadfile("jcamsys/jcamsys_camerasettings.dat",(char*)jcsm->cs,sizeof(jcsm->cs));
	if (r==sizeof(jcsm->cs))
	{
		printf("%s(%05d): loaded jcamsys/jcamsys_camerasettings.dat\n",PROGNAME,getpid());
		for (i=1;i<JC_MAX_CAMS+1;i++)
		{
			// If these are specified on command line then override values loaded from file
			//if (jcsm->gfps>0)
				//jcsm->cs[i].update_rate_ms=(1000/jcsm->gfps);
			//if (jcsm->gwidth>0)
				//jcsm->cs[i].requested_width=jcsm->gwidth;
			//if (jcsm->gheight>0)
				//jcsm->cs[i].requested_height=jcsm->gheight;
			//if (jcsm->gquality>0)
				//jcsm->cs[i].requested_height=jcsm->gquality;

			if (jcsm->cs[i].id[0]!=0)
			{
				printf("%s(%05d): Camera %02d RW:%d\tRH:%d\tW:%d\tH:%d\tQ:%d\tFPS:%d tied to ID %s\n",PROGNAME,getpid(),i,
					jcsm->cs[i].requested_width, jcsm->cs[i].requested_height, 
					jcsm->cs[i].width, jcsm->cs[i].height, 
					jcsm->cs[i].quality, (1000/jcsm->cs[i].update_rate_ms), jcsm->cs[i].id);
				fflush(stdout);
			}

			// Check this camera has the image size required by archiver
			if (jcsm->cs[i].scaledown[jcsm->se.archive_image_size]!=TRUE)
			{
				printf("%s(%05d): **WARNING** Camera %d does not have images size %d required by archiver !\n",
					PROGNAME,getpid(),i,jcsm->se.archive_image_size);
				fflush(stdout);
			}
		}
	}




	printf("%s(%05d): generating testcard, image: ",PROGNAME,getpid());
	fflush(stdout);
	for (c=1;c<JC_MAX_CAMS+1;c++)
	{
		jc_testcard(c);
		printf("%d ",c);
	}
	printf("\n");
	fflush(stdout);

	// Start archiver as child process, we always start it but if shm->archiver==FALSE it will be inactive
	pid_t pidarchiver=fork();
	if (pidarchiver==0)
		jc_archiver();

	pid_t pidsensor=fork();
	if (pidsensor==0)
		jc_sensor();

	if ( (JC_ENABLE_KX_SERVER==TRUE) & (jcsm->se.nokx!=TRUE) )						// unless configured out or -nokx
	{
		pid_t pidkx=fork();										// start key exchange server (kxserver)
		if (pidkx==0)
			jc_keyexchange(&ky);									// note service starts suspended
	}

	if (jcsm->se.nohttp!=TRUE)
	{
		pid_t pidht=fork();
		if (pidht==0)
			jc_httpd();
	}


	fd_serversock=create_listening_tcp_socket(jcsm->se.server_tcp_port);
	if (fd_serversock<=0)
	{
		printf("Error, failed to create listening server socket on port %d\n",jcsm->se.server_tcp_port);
		exit(1);
	}
	printf("%s(%05d): created listening socket, TCP port %d\n",PROGNAME,getpid(),jcsm->se.server_tcp_port);
	fflush(stdout);
	if (debug==TRUE)
		printf("%s(%05d): debug is on, expect lots of text\n",PROGNAME,getpid());

	ipbar_init(jcsm->iplist);

	while(jcsm->quit!=TRUE)
	{
		client_sock = accept(fd_serversock, (struct sockaddr *)&client, (socklen_t*)&sockaddrlen);	// parent jcamsysserver blocks here
		if (client_sock < 0)
		{
			perror("accept failed");
			exit(1);
		}

//TODO:
//if (num_active_processes<JC_MAX_CHILDREN)
// fork()
		num_active_processes++;	
                pidchild=fork();
		if (pidchild==0)
		{
			close(fd_serversock);									// close my copy of the parents socket
			int x=0;

			x=ipbar_failed_connections(jcsm->iplist, client.sin_addr.s_addr);
			if (x>=jcsm->se.ipbar_max_fails)							// has this IP failed to auth too many times?
			{
				close(client_sock);
				exit(0);
			}


			sprintf(ipaddr,"%s",(char*)inet_ntoa(client.sin_addr));
			printf("%s(%05d): connection from %s\n",PROGNAME,getpid(),ipaddr);
			if (jc_fd_blocking(client_sock,FALSE)==-1)
			{
				perror("failed to set non blocking for client_sock");
				exit(1);
			}


			// client athentication
			r=jcam_read_compare_passphrase(&ky,&client_sock,jcsm->se.filename_passwd,3000);
			if (r!=JC_OK)										// not valid auth
			{
				if (r==JC_ERR_TIMEOUT)
					printf("%s(%05d): IP:%s timeout waiting for pwlen\n",PROGNAME,getpid(),ipaddr);
				x=ipbar_add_fail(jcsm->iplist,client.sin_addr.s_addr);				// record the fail attempt
				if (x<jcsm->se.ipbar_max_fails)
					printf("%s(%05d): IP:%s failed authentication, attempt=%d\n",PROGNAME,getpid(),ipaddr,x);
				else	printf("%s(%05d): IP:%s failed authentication %d times, will now be ignored\n",PROGNAME,getpid(),ipaddr,x);
				fflush(stdout);
				close(client_sock);
				exit(0);
			}
			printf("%s(%05d): athenticated %s\n",PROGNAME,getpid(),ipaddr);
			fflush(stdout);
			ipbar_clear_entry(jcsm->iplist,client.sin_addr.s_addr);					// Authed ok, remove fail count
			sprintf(st,"AOK");									// tell client auth ok
			if (jc_sockwrite(client_sock,(unsigned char*)&st,strlen(st))==JC_ERR)			// send AOK 
			{											// but exit and cleanup if socket closed
				printf("%s(%05d) socket error, exiting %d....\n",PROGNAME,getpid(),feeder_cam); 
				fflush(stdout);
				exit(0);
			}

			// Will force a send of all camera settings if role=console
			for (i=1;i<JC_MAX_CAMS+1;i++)
				pcs_changed[i]=jcsm->cs_changed[i]+20;
			pse_changed=jcsm->se_changed+20;
			parchive_comment_changed=jcsm->archive_comment_changed+20;


			// Clear local copy of sensor_state, now we will generate messages for lots of non zero values when new user connects
			poll_sensor_change((struct jcamsys_sensorstate*)&jcsm->senss, client_sock, TRUE);
			clear_messagemask((struct jcamsys_messagemask*)&mm);					// set default mask for new connection
			while (quit!=TRUE)
				jc_server(client_sock,ipaddr);
			close(client_sock);
			printf("%s(%05d): disconnected IP:%s\n",PROGNAME,getpid(),ipaddr);			// our time together is over
			if (feeder_cam>0)									// where you a camera ?
				deregister_cam(feeder_cam);							// then unhook camera from system
			printf("%s(%05d): exiting\n",PROGNAME,getpid()); 
			exit(0);
		}
		close(client_sock);
	}

	// Clean shutdown
	printf("%s(%05d): closing down\n",PROGNAME,getpid());
	munmap (jcsm,sizeof(struct sharedmem));
	close(fd_serversock);
	return(0);
}

