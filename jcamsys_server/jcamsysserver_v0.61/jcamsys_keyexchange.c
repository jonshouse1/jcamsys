// jcamsys_keyexchange.c

#define PROGNAME        "jc_keyexchange"
#define VERSION         "0.1"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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

extern int sockaddrlen;
int fd_serversock=-1;
int fd_client_sock=-1;
int kxport=0;
unsigned long int starttime=0;

extern struct sharedmem* jcsm;

void jc_keyexchange(struct jcamsys_key* key)
{
	int kx=100;
	struct sockaddr_in client;
        char ipaddr[16];
	char st[2048];
	int r=0;
	int i=0;
	unsigned char b;
	uint16_t kl=0;

	fflush(stdout);
	printf("%s(%05d): started process\n",PROGNAME,getpid());

	while (jcsm->quit!=TRUE)
	{
		if (kx!=jcsm->se.enable_keyexchange)							// flag changed state?
		{
			if (jcsm->se.enable_keyexchange==TRUE)
				printf("%s(%05d): starting\n",PROGNAME,getpid());
			else	
			{
				printf("%s(%05d): suspended\n",PROGNAME,getpid());
				if (fd_serversock>0)
				{
					close(fd_serversock);
					fd_serversock=-1;
				}
			}
			kx=jcsm->se.enable_keyexchange;
		}


		if (jcsm->se.enable_keyexchange==TRUE)
		{
			if (fd_serversock<0)
			{
				kxport=jcsm->se.server_tcp_port+10;							// port is always base port +10
				fd_serversock=create_listening_tcp_socket(kxport);
				if (fd_serversock>0)
				{
					jc_fd_blocking(fd_serversock,FALSE);						// set non blocking
					jcsm->ss.kx_active=TRUE;
					starttime=current_timems();
					printf("%s(%05d): Listening TCP port %d\n",PROGNAME,getpid(),kxport);
					fflush(stdout);
				}
				else
				{
					printf("%s(%05d): Error opening listening socket\n",PROGNAME,getpid());
					fflush(stdout);
					jcsm->ss.kx_active=FALSE;
				}
			}
			else
			{
				// accept and service connections
				fd_client_sock = accept(fd_serversock, (struct sockaddr *)&client, (socklen_t*)&sockaddrlen);
                		if (fd_client_sock > 0)
				{
                			int pidchild=fork();
                			if (pidchild==0)
                			{
						if (jcsm->se.enable_keyexchange!=TRUE) 
							exit(0);
                        			close(fd_serversock);								// close my copy of the parents socket
                        			sprintf(ipaddr,"%s",(char*)inet_ntoa(client.sin_addr));
						printf("%s(%05d): Connection %s\n",PROGNAME,getpid(),ipaddr);
						fflush(stdout);
						bzero(&st,sizeof(st));
						r=read(fd_client_sock,&st,sizeof(st));
						if ( (r==2) & (strcmp(st,"?K")==0) )						// got request?
						{
							printf("%s(%05d): Got valid key request, sending shared key %s\n",PROGNAME,getpid(),ipaddr);
							fflush(stdout);
							kl=key->keylength;
							write(fd_client_sock,&kl,2);						// send key length
							for (i=0;i<key->keylength;i++)
							{
								b=key->xorkey[i];
								write(fd_client_sock,&b,1);
							}
						}
						shutdown(fd_client_sock,3);							// key is sent
						r=read(fd_client_sock,&st,0);
						close(fd_client_sock);
						exit(0);
					}
				}
				//else	perror("accept failed");
			}
//printf("t=%d  tout=%d\n",current_timems()-starttime, (uint32_t)jcsm->se.kx_timeoutms); fflush(stdout);

			if (current_timems()-starttime > jcsm->se.kx_timeoutms)
			{
				jcsm->ss.kx_active=FALSE;
				close(fd_serversock);
				fd_serversock=-1;
				printf("%s(%05d): timeout \n",PROGNAME,getpid());
				fflush(stdout);
				jcsm->ss.kx_active=FALSE;
				jcsm->se.enable_keyexchange=FALSE;
				jcsm->se_changed++;
			}
		}
		sleep_ms(100);
	}


	printf("%s(%05d): exiting\n",PROGNAME,getpid());
	exit(0);
}




