// JA Generic Discovery
//
// Listens to YAFDP (generic discovery protocol) messages, maintains a list of hosts and services
//

		// TODO:   Rewrite this to use jcsm->se.server_tcp_port,  http_server_tcp_port, kx_server_tcp_port

#define PROGNAME        "jc_yafdpd"
#define VERSION 	"0.06"

#define TRUE    1
#define FALSE   0

#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h> 
#include <sys/resource.h>
#include <stdlib.h>
#include <math.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <stropts.h>
#include <sgtty.h>

#include "yet_another_functional_discovery_protocol.h"
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



extern int verbose;
extern int debug;
extern struct sharedmem* jcsm;

char yafdp_manufacturer[17];
char yafdp_modelname[33];
char yafdp_device_description[33];
char yafdp_location[33];




// -1 entry indicates end of list
const char * yafdp_servicelist[][5] = {   
                                          {"0","8282","jcamsysserver","","" },
                                          {"-1","-1","-1","","" }   
                                      };

int yafdp_count_services()
{
        int i;

        for (i=0;i<YAFDP_MAX_SERVICES;i++)
        {
                if (strcmp(yafdp_servicelist[i][0],"-1")==0)
                        return(i);                                                                              // hit end of list
        }
	return(-1);
}



void yafdpserver()
{
	int number_of_services=-1;
	int drxfd=-1;
	int numbytes=0;
	struct sockaddr_in serv_addr;
	unsigned int addr_len=sizeof(serv_addr);
	char ipaddr[32];
	char txbuf[2048];
	unsigned int r;

        // Allocate a buffer, place two structures ontop of that buffer
        char rbuffer[8192];
        struct yafdp_request_devices *yafdprd=(struct yafdp_request_devices*)&rbuffer;
        struct yafdp_request_services *yafdprs=(struct yafdp_request_services*)&rbuffer;
        struct yafdp_request_service *yafdpr=(struct yafdp_request_service*)&rbuffer;

        // Overlay these over the buffer for the transmitted reply
        struct yafdp_reply_device *yafdpreplydev=(struct yafdp_reply_device*)&txbuf;
        struct yafdp_reply_service *yafdpreplyser=(struct yafdp_reply_service*)&txbuf;


        if (number_of_services<0)
                number_of_services=yafdp_count_services();

	printf("%s(%05d): started yafdod process, services=%d\n",PROGNAME,getpid(),number_of_services); 
	fflush(stdout);

	drxfd=yafdp_setup_receive_socket(TRUE, YAFDP_DISCOVRY_PORT, TRUE, FALSE);


	strcpy(yafdp_manufacturer,"jonshouse");
	strcpy(yafdp_modelname,"jcamsysserver");
	strcpy(yafdp_device_description,"MJPEG camera server");
	strcpy(yafdp_location,"");

	while (jcsm->quit!=TRUE)
	{
               	bzero(rbuffer,sizeof(rbuffer));
                numbytes = recvfrom (drxfd, &rbuffer, sizeof(rbuffer), 0, (struct sockaddr *)&serv_addr, &addr_len);
                if (numbytes>0)
                {
                        if (strcmp(yafdprs->magic,YAFDP_MAGIC)==0)
                        {
                                sprintf(ipaddr,"%s",(char*)inet_ntoa(serv_addr.sin_addr));
                                switch (yafdprd->ptype) // was rd
                                {
                                        case YAFDP_TYPE_DISCOVERY_REQUEST_DEVICES :
						if (verbose==TRUE)
                                                	printf("%s(%05d): %s got YAFDP_TYPE_DISCOVERY_REQUES_DEVICES\n",PROGNAME,getpid(),ipaddr);
                                		bzero(&txbuf,sizeof(struct yafdp_reply_device));
                                		// Build reply packet
                                		strcpy(yafdpreplydev->magic,YAFDP_MAGIC);
                                		yafdpreplydev->pver[0] = YAFDP_PVER_MAJ;
                                		yafdpreplydev->pver[1] = YAFDP_PVER_MINOR;
                                		yafdpreplydev->ptype = YAFDP_TYPE_DISCOVERY_REPLY_DEVICE;
                                		yafdpreplydev->request_handle = yafdprd->request_handle;
                                		yafdpreplydev->number_of_services=number_of_services;
                                		strcpy(yafdpreplydev->device_manufacturer,yafdp_manufacturer);
                                		strcpy(yafdpreplydev->device_modelname,yafdp_modelname);
                                		strcpy(yafdpreplydev->device_description,yafdp_device_description);
                                		strcpy(yafdpreplydev->device_location,yafdp_location);
						udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_device), (char*)&ipaddr, YAFDP_DISCOVRY_PORT, FALSE);
                        		break;

					// Client requested a service by list sequence
					case YAFDP_TYPE_DISCOVERY_REQUEST_SERVICES :
						if (verbose==TRUE)
							printf("%s(%05d): got DISCOVERY_REQUEST_SERVICES\n",PROGNAME,getpid());
						bzero(&txbuf,sizeof(struct yafdp_reply_service));
						if (yafdprs->list_sequence<number_of_services)
						{
							if (debug==TRUE)
                                                		printf("%s(%05d): sending service index=%d\n",PROGNAME,getpid(),r);
							bzero(&txbuf,sizeof(yafdpreplyser));
							r=yafdprs->list_sequence;
							strcpy(yafdpreplyser->magic,YAFDP_MAGIC);
							yafdpreplyser->list_sequence = r;
							yafdpreplyser->pver[0] = YAFDP_PVER_MAJ;
							yafdpreplyser->pver[1] = YAFDP_PVER_MINOR;
							yafdpreplyser->ptype = YAFDP_TYPE_DISCOVERY_REPLY_SERVICE;
							yafdpreplyser->request_handle = yafdprs->request_handle;
							yafdpreplyser->list_sequence = yafdprs->list_sequence;
			 				yafdpreplyser->udp_port = atoi(yafdp_servicelist[r][0]);
			 				yafdpreplyser->tcp_port = atoi(yafdp_servicelist[r][1]);
        						strcpy(yafdpreplyser->service_protocol_name_short, yafdp_servicelist[r][2]);
        						strcpy(yafdpreplyser->u1, yafdp_servicelist[r][3]);
        						strcpy(yafdpreplyser->u2, yafdp_servicelist[r][4]);
						        udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_device), (char*)&ipaddr, YAFDP_DISCOVRY_PORT, FALSE);
						}
					break;

					// Do we have a given named servie ?
					case YAFDP_TYPE_DISCOVERY_REQUEST_SERVICE :							// look for a single service by name
						if (verbose==TRUE)
							printf("%s(%05d): got DISCOVERY_REQUEST_SERVICE\n",PROGNAME,getpid());
						for (r=0;r<number_of_services;r++)							// check all services 
						{
							if (debug==TRUE)
							{
								printf("%s(%05d): asked for service_protocol_name_short=%s\n",PROGNAME,getpid(),yafdpr->service_protocol_name_short); 
								fflush(stdout);
							}
        						if (strcmp(yafdpr->service_protocol_name_short,yafdp_servicelist[r][2])==0)	// Match by name ?
							{
								bzero(&txbuf,sizeof(yafdpreplyser));
								if (debug==TRUE)
									printf("%s(%05d): sending service index=%d\n",PROGNAME,getpid(),r);
								strcpy(yafdpreplyser->magic,YAFDP_MAGIC);
								yafdpreplyser->list_sequence = r;
								yafdpreplyser->pver[0] = YAFDP_PVER_MAJ;
								yafdpreplyser->pver[1] = YAFDP_PVER_MINOR;
								yafdpreplyser->ptype = YAFDP_TYPE_DISCOVERY_REPLY_SERVICE;
								yafdpreplyser->request_handle = yafdpr->request_handle;
			 					yafdpreplyser->udp_port = atoi(yafdp_servicelist[r][0]);
			 					yafdpreplyser->tcp_port = atoi(yafdp_servicelist[r][1]);
        							strcpy(yafdpreplyser->service_protocol_name_short, yafdp_servicelist[r][2]);
        							strcpy(yafdpreplyser->u1, yafdp_servicelist[r][3]);
        							strcpy(yafdpreplyser->u2, yafdp_servicelist[r][4]);
						        	udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_device), (char*)&ipaddr, YAFDP_DISCOVRY_PORT, FALSE);
								if (debug==TRUE)
									printf("%s(%05d): sent reply to %s\n",PROGNAME,getpid(),ipaddr); 
							}
						}
					break;


                                        case YAFDP_TYPE_DISCOVERY_REPLY_DEVICE :
						if (debug==TRUE)
                                                	printf("%s(%05d): got YAFDP_TYPE_DISCOVERY_REPLY_DEVICE from %s,that should not happen\n",PROGNAME,getpid(),ipaddr);
                                        break;

                                        case YAFDP_TYPE_DISCOVERY_REPLY_SERVICE :
						if (debug==TRUE)
                                                	printf("%s(%05d): got YAFDP_TYPE_DISCOVERY_REPLY_SERVICE from %s,that should not happen\n",PROGNAME,getpid(),ipaddr);
                                        break;
                                }
                                fflush(stdout);
                        }
                }
		usleep(100 * 1000);							// numms * 1ms
	}
	close(drxfd);
	printf("%s(%05d): exiting\n",PROGNAME,getpid()); 
	exit(0);
}

