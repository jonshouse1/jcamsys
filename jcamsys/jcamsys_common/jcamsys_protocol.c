// jcamsys_protocol.c
//
// This code is used by client and server
// messages (msg) travel in either direction
// requests (req) tend to made by clients and fulfilled by the server


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_network.h"


static unsigned char sendbuf[JC_MAX_MSGLEN];


// Display the non binary fields from a jcamsts_image structure
void print_struct_jcamsys_image(struct jcamsys_image* jci)
{
	char st[1024];
	struct tm *info;
	unsigned long t=jci->timestamp/1000;
	st[0]=0;

	printf("image_type\t= %d\n",jci->image_type);
	printf("optional_crc32\t= %08X\n",jci->optional_crc32);
	printf("frame_number\t= %d\n",jci->frame_number);
	printf("update_rate_ms\t= %d\n",jci->update_rate_ms);
	printf("sipaddr\t\t= %s\n",jci->sipaddr);	
	printf("img\t\t= %d\n",jci->img);
	printf("cam\t\t= %d\n",jci->cam);
	printf("crypted\t\t= %d\n",jci->crypted);
	printf("bytes\t\t= %d\t(%dk)\n",jci->cbytes,jci->cbytes/1024);
	printf("width\t\t= %d\n",jci->width);
	printf("height\t\t= %d\n",jci->height);
	printf("sid\t\t= %s\n",jci->sid);

	info=localtime((time_t*)&t);

	strftime(st, sizeof(st), "%Y-%m-%d %H:%M:%S", info);
	printf("timestamp\t= %llu\t%s:",(long long unsigned int)jci->timestamp,st);
	sprintf(st,"%llu",(long long unsigned int)jci->timestamp);
	printf("%s",st+(strlen(st)-3));		// print last 3 digits only

	printf("\n");
	fflush(stdout);
}


// convert millsecond back into datetime
//struct tm* GetTimeAndDate(unsigned long long milliseconds)
//{
    //time_t seconds = (time_t)(milliseconds/1000);
    //if ((unsigned long long)seconds*1000 == milliseconds)
        //return localtime(&seconds);
    //return NULL; // milliseconds >= 4G*1000
//}




// if user reqid==0 then use a global one for sending, return is the reqid
int jc_nextreqid(uint16_t user_supplied_reqid)
{
	uint16_t reqid=0;
	static uint16_t  sendreqid=0;

	if (user_supplied_reqid==0)
	{
		sendreqid++;
		reqid=sendreqid;
	}
	else	reqid=user_supplied_reqid;
	return(reqid);
}

// valid returns 0, not vali -1
int valid_image_cam(int img, int cam, char *pn, char *ipaddr, int silent)
{
	if ( (img<0) | (img>JC_MAX_IMAGES) )
	{
		if (silent!=TRUE)
		{
			printf("%s(%05d): IP:%s -  img %d out of range\n",pn,getpid(),ipaddr,img);
			fflush(stdout);
			return(-1);
		}
	}
	if ( (cam<0) | (cam>JC_MAX_CAMS) )
	{
		if (silent!=TRUE)
		{
			printf("%s(%05d): IP:%s -  cam %d out of range\n",pn,getpid(),ipaddr,cam);
			fflush(stdout);
			return(-1);
		}
	}
	return(0);
}


void printmm(struct jcamsys_messagemask* mx)
{
        int i=0;
        for (i=1;i<JC_MAX_CAMS+1;i++)
                if (mx->frame_changes[IFULL][i]==TRUE)
                        printf("X");
                else    printf(".");
}






//#define SOCKETBLOCKING								// Socket is opened blocking
#ifdef SOCKETBLOCKING
// For socket opened blocking
int jc_sockwrite(int fd, unsigned char* buf, int len)
{
	int w;

	w=write(fd,buf,len);
	if (w==-1)
		return(-1);
	return(len);
}



// For socket opened blocking
int jcam_read_message(int sockfd, struct jcamsys_key* key, unsigned char*buf, int bufsize, int *msgtype, uint16_t *reqid, int timeoutms)
{
	static struct jcamsys_header  hdr;					// static does matter, pointer returned to this
	static int state=0;
	static int bytestoread=0;						// target size for reading
	static int bytesread=0;							// bytes read so far
	int r=0;
	int p=0;

	*msgtype=0;
	*reqid=0;
	switch(state)
	{
		case 0:
			bytestoread=0;
			bytesread=0;
			p=jc_peekbytes(sockfd);					// how many bytes available for read() ?
			if (p<sizeof(struct jcamsys_header))
				return(0);					// idle
			r=read(sockfd,&hdr,sizeof(struct jcamsys_header));	// read header 
			if ( (hdr.magic[0]!=MAGIC0) | (hdr.magic[1]!=MAGIC1) | (hdr.magic[2]!=MAGIC2) | (hdr.magic[3]!=MAGIC3) )
			{
				p=jc_peekbytes(sockfd);	
				r=read(sockfd,buf,p);				// discard
				return(JC_ERR_BAD_MAGIC);
			}
			if (hdr.msglen>=JC_MAX_MSGLEN)				// is payload size sane ?
				return(JC_ERR_BAD_VALUE);
			bytestoread=hdr.msglen;	
			state=1;
			return(1);
		break;

		case 1:
			*reqid   = hdr.reqid;
			*msgtype = hdr.msgtype;					// show caller what message to process
			p=jc_peekbytes(sockfd);	
printf("p=%d\n"); fflush(stdout);
//if (p<10) return(1);
			if (p>bytestoread)
				p=bytestoread;
			if (p<=0)
				return(-1);

			r=read(sockfd,buf+bytesread,p);				// read payload in as big a chunks as possible
			if (r<0)
				return(JC_ERR);
			if (r==0)
			{
				state=0;					// socket closed?
				return(JC_ERR);
			}
			if (r>=1)
			{
				bytesread = bytesread + r;
				bytestoread = bytestoread - r;
			}

			if (bytestoread<=0)
			{
				state=0;					// completed read ok
				return(JC_HAVE_DATA);				// tell caller to process message
			}
			return(1);
		break;
	}
	return(0);
}

#else										// Elese socket is open non blocking


int jc_sockwrite(int fd, unsigned char* buf, int len)
{
        int w=0;
        int byteswritten=0;
        int bytestowrite=len;

        do
        {
		// return of 0 is not an error
//printf("fd=%d len=%d\n",fd,len); fflush(stdout);
                w=write(fd,buf,len);
                //printf("jc_sockwrite() write returned %d of %d\n",w,len); fflush(stdout);
                if (w==-1) 
		{
			//if (errno!=EAGAIN)			// Not an error
			//{
				//printf("jc_sockwrite - write()==-1 errno= %d %s btr=%d\n\n",errno,strerror(errno),bytestowrite);
				//fflush(stdout);
				//return(-1);
			//}
			if ( (errno==EPIPE) | (errno==EBADF) )
{
//printf("jc_sockwrite() fd=%d - EPIPE or EBADF, returning -1\n",fd); fflush(stdout);
				return(-1);
}
		}
                if (w>=1)
                {
                        byteswritten=byteswritten+w;
                        bytestowrite=bytestowrite-w;
                }
        } while (bytestowrite>0);
        return(byteswritten);
}




void drain(int sockfd, int c)
{
	char buf[1024];
	int i=0;
	
	printf("%d:DRAIN! %d \n",getpid(),c); fflush(stdout);
	for (i=0;i<1024;i++)
		read(sockfd,&buf,sizeof(buf));
}




// Read data from non blocking socket
// return values:
//	-11 (JC_ERR_BADMAGIC)
//	-1 (JC_ERR)   caller needs to close socket and start over
//
// (state)
//	0 = idle
//	1 = still reading, keep calling us
//	JC_HAVE_DATA = data to process,  *msgtype will be set, else *msgtype will be 0
//
int jcam_read_message(int sockfd, struct jcamsys_key* key, unsigned char*buf, int bufsize, int *msgtype, uint16_t *reqid, int timeoutms)
{
	static int state=0;
	static int bytestoread=0;						// target size for reading
	static int bytesread=0;							// bytes read so far
	static int starttimems=0;						// note when an operation started
	int timems=0;
	static struct jcamsys_header  hdr;					// static does matter, pointer returned to this
	int r=0;

	// Normally this follows the states through without returning
	if (state==0)
	{
		// clear any state we cary over between calls
		bzero(&hdr,sizeof(hdr));
		bytesread=0;
		bytestoread=0;
		*msgtype=0;
		r=jc_peekbytes(sockfd);						// how many bytes available for read() ?
		if (r==JC_ERR)
			return(JC_ERR);						// ioctl failed, socket closed 
		if (r<sizeof(struct jcamsys_header))				// do we have enogh bytes for a header read?
			return(0);						// nope, then idle

		// If we hot to this point then we have enough bytes pending to read the header
		r=read(sockfd,&hdr,sizeof(struct jcamsys_header));		// read header 
		if (r==0)							// end of file ?
			return(JC_ERR);						// then socket is closed
//printf("got %d expect %d ? \n",r,sizeof(struct jcamsys_header)); fflush(stdout);
		if (r==sizeof(struct jcamsys_header))				// got a header?
		{
			state=0;						// if we return here then start over next call
			if ( (hdr.magic[0]!=MAGIC0) | (hdr.magic[1]!=MAGIC1) | (hdr.magic[2]!=MAGIC2) | (hdr.magic[3]!=MAGIC3) )
			{
				drain(sockfd,1);
//printf("bad magic %c%c%c%c\n",hdr.magic[0],hdr.magic[1],hdr.magic[2],hdr.magic[3]);  fflush(stdout);
				return(JC_ERR_BAD_MAGIC);
			}
			if (hdr.msglen>=JC_MAX_MSGLEN)				// is payload size sane ?
			{
				drain(sockfd,2);
//printf("bad size\n"); fflush(stdout);
				return(JC_ERR_BAD_VALUE);
			}
			bytestoread=hdr.msglen;					// read this many bytes into 'buf'
			if (bytestoread>16)					// zero out end of buffer
				bzero((buf+bytestoread)-16,16);			// makes jpeg invalid if we do a short read
			state=1;						// state 1, read actual data
//printf("got valid header, need to read %d bytes\n",hdr.msglen); fflush(stdout);
		}
		else
		{
			printf("\n\n\njcam_read_message() -tried to read %d bytes in one chunk, got %d\n\n",
				sizeof(struct jcamsys_header),r);
			fflush(stdout);
		}
	}


	// read bytes into buf, if operation takes more than 'timeoutms' then return to caller, they will call us
	// back later to complete the operation
	if (state==1)
	{
		starttimems=current_timems();					// note the time we started reading....
		do
		{
//printf("asking for %d bytes %d avialable\n",bytestoread,jc_peekbytes(sockfd)); fflush(stdout);
if (bytesread>=bufsize)
{
   printf("Buffer overrun\n");
   exit(1);
}
			r=read(sockfd,buf+bytesread,bytestoread);		// read payload in as big a chunks as possible
//printf("got read = %d\n",r);  fflush(stdout);
			if (r==0)
			{
				state=0;					// socket closed?
				return(JC_ERR);
			}
			if (r>=1)
			{
				bytesread = bytesread + r;
				bytestoread = bytestoread - r;
//printf("hdr.msglen=%d\tbytestoread=%d\tbytesread=%d\n",hdr.msglen,bytestoread,bytesread); fflush(stdout);
			}
			usleep(10);
			timems=current_timems()-starttimems;			// time we have been in this loop
		} while ( (bytestoread>0) & (timems<timeoutms) );		// while more to read and not timed out
		if ( (timems>=timeoutms) & (bytestoread>0) )			// did we timeout ?
{
//JA suspect
//printf("timeout reading ..\n"); fflush(stdout);
			return(JC_TIMEOUT);					// we can still complete the read later
}
		if (bytestoread<=0)
		{
//printf("payload read ok, got %d\n",bytesread); fflush(stdout);
			*reqid   = hdr.reqid;
			*msgtype = hdr.msgtype;					// show caller what message to process
			state=0;						// completed read ok
			return(JC_HAVE_DATA);					// tell caller to process message
		}
	}
	return(state);
}

#endif


int jcamsys_img_headerlen( struct jcamsys_image* jci )
{
	// length = end - start
	int headerlen	= ((uintptr_t)&jci->cdata - (uintptr_t)&jci->sipaddr[0]);
	return (headerlen);
}



// copy jcamsys_image structure
// from source (jcis) to destination (jcid), fill in the IP address we received it from
// retuns the number of bytes copied
// if non blank ipaddr is passed it is copied into the destination struct
int jc_image_copy( struct jcamsys_image* jcid, struct jcamsys_image* jcis)
{
	int bytes = jcamsys_img_headerlen( jcis ) + jcis->cbytes;
//printf("jc_image_copy() bytes=%d\n",bytes);
	memcpy(jcid, jcis, bytes);

	return(bytes);
}






// Write a header telling the receiver how large the data block that follows is
// if reqid is passed in then that value is used, if 0 then one will be assigned
// returns the request id,  header is never sent alone so the next write along
// will detect dropped socket if needed
int jc_send_header(int sockfd, int msgtype, int msglen, uint16_t reqid)
{
	struct jcamsys_header	hdr;
	int w __attribute__((unused)) =0;		// might use it later

	if (sockfd<0)
		return(JC_ERR);

	bzero(&hdr,sizeof(struct jcamsys_header));
	hdr.magic[0]=MAGIC0;
	hdr.magic[1]=MAGIC1;
	hdr.magic[2]=MAGIC2;
	hdr.magic[3]=MAGIC3;
	hdr.msglen	= msglen;
	hdr.msgtype	= msgtype;
	hdr.reqid 	= jc_nextreqid(reqid);

	w=write(sockfd,&hdr,sizeof(struct jcamsys_header));	// should do *something*
	//if (w==-1)
		//return(JC_ERR);							// socket closed?
	//return(JC_OK);
	return(hdr.reqid);
}




// img=image index, 0=preview, 1=full
// retuns request id back, or -1 on socket error
int jc_request_image(struct jcamsys_key* key, int sockfd, uint16_t reqid, int img, int cam)
{
	struct jcamsys_req_image*	reqi =(struct jcamsys_req_image*)sendbuf;
	int rid=0;

	reqi->cam	= cam;
	reqi->img	= img;				// image size 0=largest

	rid=jc_send_header(sockfd, JC_REQ_IMAGE, sizeof(struct jcamsys_req_image),reqid);
	if (jc_sockwrite(sockfd, (unsigned char*)&sendbuf, sizeof(struct jcamsys_req_image))<0)
		return(-1);
	return(rid);
}



// If the image is being passed around then it may or may not be encrypted.
// call this to ensure that it is in desired state
void jc_image_force_crypted(struct jcamsys_key* key, struct jcamsys_image* jci)
{
	if (jci->crypted==TRUE)
		return;
	jcam_crypt_buf(key, jci->cdata, sizeof(jci->cdata), jci->cbytes);
	jci->crypted=TRUE;
}


void jc_image_force_uncrypted(struct jcamsys_key* key, struct jcamsys_image* jci)
{
	if (jci->crypted!=TRUE)
		return;
	jcam_crypt_buf(key, jci->cdata, sizeof(jci->cdata), jci->cbytes);
	jci->crypted=FALSE;
}


// assign values to most things except cdata, optionally add a CRC
void jc_image_prepare(struct jcamsys_key* key, struct jcamsys_image* jci, int img, int cam, int image_type, int width, int height, int cbytes, char *sid, int frame_number)
{
	bzero( &jci->sipaddr[0], sizeof(jci->sipaddr));	// receiver populates
	jci->img          = img;
	jci->cam          = cam;
	jci->crypted	  = FALSE;		// Assume buffer in non crypted
	jci->image_type   = image_type;
	jci->width        = width;
	jci->height       = height;
	jci->cbytes       = cbytes;
	jci->frame_number = frame_number;
	jci->optional_crc32 = 0;
	bzero (jci->sid, sizeof(jci->sid));
	jci->sipaddr[0]	  = 0;
	strcpy(jci->sid, sid);
	jci->sid[JC_IDLEN]=0;			// ensure always terminated
	jci->cdata[0]=0;
}



// returns reqid or -1,  reqid is 16 bit unsigned so an int can cope with -1 or 0..65535
int jc_msg_image(struct jcamsys_key* key, struct jcamsys_image* jci, int sockfd, int docrc, uint16_t reqid)
{
	int rid=0;
	int headerlen = jcamsys_img_headerlen( jci );
	if (sockfd<0)
		return(JC_ERR);

	jci->optional_crc32 = 0;
	if (docrc==TRUE)
	{
		if (jci->optional_crc32==0)
		{
			jc_image_force_uncrypted(key, jci);
			jci->optional_crc32 = rc_crc32(0, (char*)&jci->cdata, jci->cbytes);
		}
	}

	// date/time stamp to ms accuracy as it was sent
	jci->timestamp = current_timems();

	// image sent over the socket is always encrypted
	jc_image_force_crypted(key, jci);

	// cdata[] is defined as big, restrict the socket write to actual number
	// of bytes in cdata
	rid=jc_send_header(sockfd, JC_MSG_IMAGE, headerlen+jci->cbytes, reqid);
	if (jc_sockwrite(sockfd, (unsigned char*)jci, headerlen+jci->cbytes)<0)
		return(-1);
	return(rid);
}







// For a given type of sensor what is active with the server
int jc_msg_sens_active(struct jcamsys_key* key, struct jcamsys_sensorstate* ss, int sockfd, uint16_t reqid, int senstype)
{
	int rid=0;
	struct jcamsys_sens_active* msgs =(struct jcamsys_sens_active*)&sendbuf;
	int i=0;

	//printf("jc_msg_sens_active()\n"); fflush(stdout);
	msgs->senstype	= senstype;	

	if (sockfd<0)
		return(JC_ERR);
	if (senstype>=JC_MAX_SENSTYPE)
		return(0);

	for (i=0;i<=JC_MAX_SENSORS+1;i++)
		msgs->sensor_active[i]=ss->sensor_active[senstype][i];

	rid=jc_send_header(sockfd, JC_MSG_SENS_ACTIVE, sizeof(struct jcamsys_sens_active), reqid);

	if (jc_sockwrite(sockfd, (unsigned char*)&sendbuf, sizeof(struct jcamsys_sens_active))<0)
		return(-1);
	return(rid);
}



// Send sensor value change, all values for a sensor of type 'senstype'
int jc_msg_sens_value(struct jcamsys_key* key, struct jcamsys_sensorstate* ss, int sockfd, uint16_t reqid, int senstype)
{
	int rid=0;
	struct jcamsys_sens_value*  msgv =(struct jcamsys_sens_value*)&sendbuf;
	int i=0;

	msgv->senstype	= senstype;	

	if (sockfd<0)
		return(JC_ERR);
	if (senstype>=JC_MAX_SENSTYPE)
		return(0);

	for (i=0;i<=JC_MAX_SENSORS+1;i++)
	{
		msgv->sensor_fvalue[i]=ss->sensor_fvalue[senstype][i];
		msgv->sensor_ivalue[i]=ss->sensor_ivalue[senstype][i];
		strcpy(msgv->sensor_cvalue[i],ss->sensor_cvalue[senstype][i]);
	}

	rid=jc_send_header(sockfd, JC_MSG_SENS_VALUE, sizeof(struct jcamsys_sens_value),reqid);
	if (jc_sockwrite(sockfd, (unsigned char*)&sendbuf, sizeof(struct jcamsys_sens_value))<0)
		return(-1);
	return(rid);
}


// cam has changed frame number, send a short message
int jc_msg_frame_number(struct jcamsys_key* key, uint16_t frame_number, int sockfd, int img, int cam)
{
	int rid=0;
	struct jcamsys_frame_number*        msgn    = (struct jcamsys_frame_number*)&sendbuf;
	msgn->img	   = img;
	msgn->cam	   = cam;
	msgn->frame_number = frame_number;
	//printf("MSGFRAMENO\n"); fflush(stdout);

	if (sockfd<0)
		return(JC_ERR);
	if (img!=IFULL)
		return(0);

	rid=jc_send_header(sockfd, JC_MSG_FRAME_NUMBER, sizeof(struct jcamsys_frame_number), 0);
	if (jc_sockwrite(sockfd, (unsigned char*)&sendbuf, sizeof(struct jcamsys_frame_number))<0)
		return(-1);
	return(rid);
}


// send messagemask message
int jc_msg_messagemask(struct jcamsys_key* key, int sockfd, uint16_t reqid, struct jcamsys_messagemask* mm)
{
	int rid=0;
	struct jcamsys_messagemask ml;				// local copy

	if (sockfd<0)
		return(JC_ERR);

	memcpy(&ml,mm,sizeof(struct jcamsys_messagemask));	// take a copy

	//printf("jc_msg_messagemask() \n"); printmm(&ml); fflush(stdout);

	rid=jc_send_header(sockfd, JC_MSG_MESSAGEMASK, sizeof(struct jcamsys_messagemask), reqid);
	if (jc_sockwrite(sockfd, (unsigned char*)&ml, sizeof(struct jcamsys_messagemask))<0)
		return(-1);
	return(rid);
}



int jc_msg_dts(struct jcamsys_key* key, int sockfd, char *dts)
{
	int rid=0;
	struct jcamsys_dts dt;

	if (sockfd<0)
		return(JC_ERR);
	strcpy(dt.dts,dts);
	rid=jc_send_header(sockfd, JC_MSG_DTS, sizeof(struct jcamsys_dts),0);
	if (jc_sockwrite(sockfd, (unsigned char*)&dt, sizeof(struct jcamsys_dts))<0)
		return(-1);
	return(rid);

}



// Typically used to tell the server the role the connected device has
int jc_msg_register(struct jcamsys_key* key, int sockfd, char*id, int role)
{
	int rid=0;
	struct jcamsys_register	ri;

//printf("jc_msg_register()\n"); fflush(stdout);
	bzero(&ri,sizeof(struct jcamsys_register));
	ri.role = role;
	strcpy(ri.id,id);
	
	rid=jc_send_header(sockfd, JC_MSG_REGISTER, sizeof(struct jcamsys_register),0);
	if (jc_sockwrite(sockfd, (unsigned char*)&ri, sizeof(struct jcamsys_register))<0)
		return(-1);
	return(rid);
}




int jc_msg_camerasettings(struct jcamsys_key* key, struct jcamsys_camerasettings* cs, int sockfd)
{
//printf("jc_msg_camerasettings()\n"); fflush(stdout);
	int rid=0;
	rid=jc_send_header(sockfd, JC_MSG_CAMERASETTINGS, sizeof(struct jcamsys_camerasettings),0);
	if (jc_sockwrite(sockfd, (unsigned char*)cs, sizeof(struct jcamsys_camerasettings))<0)
		return(-1);
	return(rid);
}



