// jcamsys_sensor
// Monitor for sensors sending UDP data, feed that into the server


#define SENSOR_UDP_PORT			2080
#define OS_SENS_TIMEOUS_MS		120 * 1000

#define PROGNAME        "jc_sensor"
#define VERSION         "0.2"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>

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
#include "jcamsys_sensors_prot.h"
#include "id.h"


extern int verbose;
extern int debug;

extern struct sharedmem* jcsm;

int port=0;
int fdsock=-1;


// onsight PIR message format, one instance called ons_message
struct __attribute__((packed)) osm
{
	char senderpid[8];
        char serialnumber[12];
        char datetime[30];
        char messagetype[16];
        char message[1900];
};


int udp_sensor_receive_socket(int port)
{
	static struct sockaddr_in recvaddr; 
	int bret;
	int udpsockfd=-1;
	udpsockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (udpsockfd<0)
	{
		perror("error, could not create socket");
		exit(1);
	}

	// Setuop socket, specify listening interfaces etc
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = INADDR_ANY;
	memset(recvaddr.sin_zero,'\0',sizeof (recvaddr.sin_zero));

	// Put socket in non blcoking mode
	int flags = fcntl(udpsockfd, F_GETFL);                                                          // Get the sockets flags
	flags |= O_NONBLOCK;                                                                            // Set NONBLOCK flag
	if (fcntl(udpsockfd, F_SETFL, flags) == -1)                                                     // Write flags back
	{
                perror("error,fcnctl failed - could not set socket to nonblocking");
                exit(1);
	}

        // Bind to socket, start socket listening
	int optval=1;
	if (setsockopt(udpsockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0)
	{
		printf("SO_REUSEPORT failed, not available on this kernel ?\n");
	}
	bret = bind(udpsockfd, (struct sockaddr*) &recvaddr, sizeof (recvaddr));
	if (bret < 0)
	{
		printf("(%d) for port (%d)\n",bret,port);
		perror("bind failed, only one process can bind at a time");
		exit(1);
	}
	return(udpsockfd);
}



// Check for any UDP data from sensors
void sensor_poll(int fd)
{
	struct osm	msg;
	//unsigned char st[8192];
	int r=0;
	int i=0;
	int t=0;
	uint32_t	tnow;
	uint32_t	tdiff;
	float f;

	while (jcsm->senss.initialised!=TRUE)
	{
		printf("z"); fflush(stdout);
		sleep(1);						// wait for main processes to init shared memory
	}


	// timeout inactive
	tnow=current_timems();
	for (i=0;i<JC_MAX_SENSORS+1;i++)							// for every sensor of that type
	{
		if (jcsm->senss.sensor_active[JC_SENSTYPE_TEMP][i]==TRUE)			// if its active
		{
			tdiff = tnow - jcsm->senss.sensor_change_time_ms[JC_SENSTYPE_TEMP][i];	// how many ms since last change and now?
			//printf("temp %d last heard %d seconds ago\n",i,tdiff/1000);  fflush(stdout);
			if (tdiff>OS_SENS_TIMEOUS_MS)						// timeout ?
			{
				jcsm->senss.sensor_active[JC_SENSTYPE_TEMP][i]=FALSE;		// mark inactive
				jcsm->senss.sensor_active_changed[JC_SENSTYPE_TEMP]++;		// active[] array has changed
			}
		}
	}



	r=read(fd,(unsigned char*)&msg,sizeof(msg));
	if (r>0)
	{
		//printf("senderpid [%s]\n",msg.senderpid);
        	//printf("serialnumber [%s]\n",msg.serialnumber);
        	//printf("datetime [%s]\n",msg.datetime);
        	//printf("messagetype [%s]\n",msg.messagetype);
        	//printf("message [%s]\n",msg.message);
		if (verbose==TRUE)
			printf("%s(%05d): Got UDP message type=[%s] msg=[%s]\n",PROGNAME,getpid(),msg.messagetype,msg.message);

		if (strcmp(msg.messagetype,"TEMPSENS")==0)
		{
			sscanf(msg.message,"T%d=%f",&t,&f); 
			if (verbose==TRUE)
			{
				printf("%s(%05d): n=%d\tf=%02f\n",PROGNAME,getpid(),t,f);
				fflush(stdout);
			}


			if ( (t>0) & (t<JC_MAX_SENSORS) )
			{
				jc_sensor_mark_active(&jcsm->senss, JC_SENSTYPE_TEMP, t);

				jcsm->senss.sensor_fvalue[JC_SENSTYPE_TEMP][t]=f;		// update the actual temperature
				jcsm->senss.sensor_value_changed[JC_SENSTYPE_TEMP]++;		// value for sensor has changed

				// Update change times, millisecond one and text one. Keep current and previous values
				jc_sensor_mark_time(&jcsm->senss, JC_SENSTYPE_TEMP, t);
				jcsm->senss.sensor_change_ptime_ms[JC_SENSTYPE_TEMP][t] = jcsm->senss.sensor_change_time_ms[JC_SENSTYPE_TEMP][t];

				jc_set_time_ms(&jcsm->senss.sensor_change_time_ms[JC_SENSTYPE_TEMP][t]);
				//strcpy(jcsm->senss.sensor_change_pdatetime[JC_SENSTYPE_TEMP][t], jcsm->senss.sensor_change_datetime[JC_SENSTYPE_TEMP][t]);
				//datetime(&jcsm->senss.sensor_change_datetime[JC_SENSTYPE_TEMP][t],FALSE);
			}	
		}
	}
}






void jc_sensor()
{
	int port=SENSOR_UDP_PORT;

	fflush(stdout);
	printf("%s(%05d): started sensor receiver process\n",PROGNAME,getpid());
	fdsock=udp_sensor_receive_socket(port);
	if (fdsock>0)
		printf("%s(%05d): listening for UDP messages on port %d\n",PROGNAME,getpid(),port);

	while (jcsm->quit!=TRUE)
	{
		sleep(1);
		sensor_poll(fdsock);
	}
	printf("%s(%05d): exiting\n",PROGNAME,getpid());
	exit(0);
}



