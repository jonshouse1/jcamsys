// jcamsys_httpd.c
//
// HTTP server for jcamsys.
// Server will server files from two locations, the image archive directory and jcamsys/ direcory
// to aid security any paths with "."s,  / is taken as serverpath/jcamsys/   /archive is an alias
// to the archive path.
//

#define PROGNAME        "jc_httpd"
#define VERSION         "0.2"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
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
extern struct sharedmem* jcsm;


struct sockaddr_in hcaddr;
struct sockaddr_in hsaddr;




void jc_httpd()
{
	int httpserver=-1;
	char st[2048];
	pid_t hpid;

	socklen_t l;
	struct sockaddr_in hcaddr;
	struct sockaddr_in hsaddr;
	int listenfd=-1;
	int sockfd=-1;

	signal(SIGCHLD, SIG_IGN); 
	signal(SIGHUP, SIG_IGN); 

	while (jcsm->quit!=TRUE)
	{
		if (httpserver!=jcsm->se.enable_http) 
		{
                        if (jcsm->se.enable_http==TRUE)
			{
                                sprintf(st,"%s(%05d): starting\n",PROGNAME,getpid());
				hsaddr.sin_family = AF_INET;
				hsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
				hsaddr.sin_port = htons(jcsm->se.http_server_tcp_port);

				if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
					perror("listen failed");
				if(bind(listenfd, (struct sockaddr *)&hsaddr,sizeof(hsaddr)) <0)
				{
					perror("bind failed");
					//exit(1);
				}
				if( listen(listenfd,64) <0)
				{
					perror("listen failed");
					//exit(1);
				}

				printf("%s(%05d): Listening for connctions on port %d\n",PROGNAME,getpid(),jcsm->se.http_server_tcp_port);
				fflush(stdout);
				l=sizeof(hcaddr);
                		if ((sockfd = accept(listenfd, (struct sockaddr *)&hcaddr, &l)) < 0)
				{
					perror("accept failed");
				}
				else
				{
                			hpid = fork(); 
                        		if (hpid==0) 
					{
                                		close(listenfd);
                                		//web(sockfd,hit); 
                        		} else 	close(sockfd);
                		}
			}
                        else  sprintf(st,"%s(%05d): suspended\n",PROGNAME,getpid());
                        printf("%s",st); fflush(stdout);
                        httpserver=jcsm->se.enable_http;
                }

		sleep(1);
	}	
}




