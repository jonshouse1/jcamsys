// Yet another 'functional' discovery protocol.
// (c)2018 Jonathan Andrews (jon@jonshouse.co.uk)

// Version 0.16
// Last changed 27 Mar 2018

#define TRUE    1
#define FALSE   0

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h> 

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "yet_another_functional_discovery_protocol.h"


// If its missing then its unlikey to work, but hey ho lets give it a go
#ifndef SO_REUSEPORT 
	//#warning SO_REUSEPORT was not defined, it may not work on this machine
	#define SO_REUSEPORT 15
#endif



void dumphex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}


int getnonzerorand()
{
        int randomnumber;
        int i;
        srand(time(NULL));                                                                                              // Seed random number generator
        i=0;
        do
        {
                randomnumber = rand() % 65535;
                if (i>20)
                {
                        fprintf(stderr,"gdp_discover: unable to get non zero random number\n");
                        exit(1);
                }
                i++;
        } while(randomnumber==0);                                                                                       // keep going until we get a non zero number
        return(randomnumber);
}



void pdots(int printdots)
{
	if (printdots==TRUE)
	{
		printf(".");
		fflush(stdout);
	}
}



struct sockaddr_in recvaddr;										// An IP address record structure
int yafdp_setup_receive_socket(int listen, int port, int silent, int dontexit)
{
        int bret;
	int udpsockfd=-1;
        // Open socket
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
        //int flags = fcntl(udpsockfd, F_GETFL);								// Get the sockets flags
        //flags |= O_NONBLOCK;										// Set NONBLOCK flag
        //if (fcntl(udpsockfd, F_SETFL, flags) == -1)							// Write flags back
        //{
                //perror("error,fcnctl failed - could not set socket to nonblocking");
                //exit(1);
        //}

	int flags = fcntl(udpsockfd, F_GETFL, 0);
	bret = fcntl(udpsockfd, F_SETFL, flags | O_NONBLOCK);
	if (bret < 0)
	{
		printf("yafdp_setup_receive_socket() failed to set O_NONBLOCK  %s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}

        // Bind to socket, start socket listening
        if (listen==TRUE)
        {
		int optval=1;
		// for this to work EVERY process binding to this socket must issue this ...
		if (setsockopt(udpsockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0)		// Allow multiple listeners on same socket
		{
			printf("SO_REUSEPORT failed, not available on this kernel ?\n");
		}
                bret = bind(udpsockfd, (struct sockaddr*) &recvaddr, sizeof (recvaddr));
                if (bret < 0)
                {
			close(udpsockfd);
			if (silent!=TRUE)
			{
                        	printf("(%d) for port (%d)\n",bret,port);
                        	perror("bind failed, only one process can bind at a time");
				return(-2);	// bind failed
			}
			if (dontexit!=TRUE)
                        	exit(1);
                }
		if (silent!=TRUE)
		{
                	fprintf(stderr,"Listening for UDP data on port %d \n",port);
                	fflush(stderr);
		}
        }
	return(udpsockfd);
}




#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast)
{
        static int sockfd=-1;
        int broadcastx=1;
        struct sockaddr_in sendaddr;
        int numbytes;

	if (sockfd<0)
	{
        	if((sockfd = socket(PF_INET,SOCK_DGRAM,0)) == -1)
        	{
                	perror("sockfd");
                	exit(1);
        	}
	}

	if (broadcast==TRUE)
	{
		if((setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&broadcastx,sizeof broadcastx)) == -1)
        	{
                	perror("setsockopt - SO_SOCKET ");
                	exit(1);
        	}
	}

        sendaddr.sin_family = AF_INET;
        sendaddr.sin_port = htons(destination_port);
        sendaddr.sin_addr.s_addr = inet_addr(destination_ip);
        memset(sendaddr.sin_zero,'\0',sizeof sendaddr.sin_zero);
        numbytes = sendto(sockfd, d, len , 0, (struct sockaddr *)&sendaddr, sizeof sendaddr);
        //close(sockfd);
	return(numbytes);
}




int yafdp_send_discovery_request_devices(int handle)
{
	struct  yafdp_request_devices gdpr;
	int numbytes=0;

	// Populate and send a discover hardware request to all hosts
	strcpy(gdpr.magic, YAFDP_MAGIC);
	gdpr.pver[0]=YAFDP_PVER_MAJ;
	gdpr.pver[1]=YAFDP_PVER_MINOR;
	gdpr.ptype=YAFDP_TYPE_DISCOVERY_REQUEST_DEVICES;
	gdpr.request_handle=handle;
        numbytes=udp_generic_send((char*)&gdpr, sizeof(gdpr), "255.255.255.255", YAFDP_DISCOVRY_PORT, TRUE);
	return(numbytes);
}



// Ask for an entry from an indexed list of services
int yafdp_send_discovery_request_services(char* destip, int list_sequence, int handle)
{
	struct  yafdp_request_services gdsr;
	int numbytes=0;

	// Populate and send a discover services request to all hosts
	strcpy(gdsr.magic, YAFDP_MAGIC);
	gdsr.pver[0]=YAFDP_PVER_MAJ;
	gdsr.pver[1]=YAFDP_PVER_MINOR;
	gdsr.ptype=YAFDP_TYPE_DISCOVERY_REQUEST_SERVICES;
	gdsr.request_handle=handle;
	gdsr.list_sequence=list_sequence;								//  0 = report all services
        numbytes=udp_generic_send((char*)&gdsr, sizeof(gdsr), (char*)destip, YAFDP_DISCOVRY_PORT, TRUE);
	return(numbytes);
}



// Ask for a single service by name, UDP broadcast discovery
int yafdp_send_discovery_request_service(char *pname, int handle)
{
	struct  yafdp_request_service gds;
	int numbytes=0;

	// Populate and send a discover services request to all hosts
	strcpy(gds.magic, YAFDP_MAGIC);
	strncpy(gds.service_protocol_name_short,pname,sizeof(gds.service_protocol_name_short));
	gds.pver[0]=YAFDP_PVER_MAJ;
	gds.pver[1]=YAFDP_PVER_MINOR;
	gds.ptype=YAFDP_TYPE_DISCOVERY_REQUEST_SERVICE;
	gds.request_handle=handle;
        numbytes=udp_generic_send((char*)&gds, sizeof(gds), "255.255.255.255", YAFDP_DISCOVRY_PORT, TRUE);
	//dumphex((char*)&gds,sizeof(gds));
	return(numbytes);
}




// Returns details of hosts  supporting the named service. Used by a client to discover a server by protocol name
// setting quick true returns the first reply
// return value is -1 Error
// -2 failed to setup listening socket, probably bind failed (busy, sleep then retry me)
// >=0 number of services found 
int yafdp_discover_service(struct yafdp_reply_service_list service_list[], char *service_protocol_name_short, int device_discov_timeoutms, int printdots, int silent, int quick)
{
	int drxfd=-1;
	struct sockaddr_in serv_addr;
        unsigned int addr_len=sizeof(serv_addr);
	char ipaddr[32];
	int p=0;	
	int dc=0;											// number of device and service records received
	int s=0;
	int i=0;
	int numbytes=0;
	char rbuffer[8192];
	int randomnumber=0;
	int founddup=FALSE;
	struct yafdp_reply_service *gdprs=(struct yafdp_reply_service*)rbuffer;

	drxfd=yafdp_setup_receive_socket(TRUE, YAFDP_DISCOVRY_PORT, silent, TRUE);				// Prepare to receive replies from devices
	if (drxfd<0)
		return(-2);
	randomnumber=getnonzerorand();
	yafdp_send_discovery_request_service(service_protocol_name_short,randomnumber);

	dc=0;
	for (p=0;p<device_discov_timeoutms;p++)
	{
		usleep(1000);										// one millisecond
		s++;
		if (s>200)
		{
			yafdp_send_discovery_request_service(service_protocol_name_short,randomnumber);
			s=0;
		}


		bzero(rbuffer,sizeof(rbuffer));
		numbytes = recvfrom (drxfd, &rbuffer, sizeof(rbuffer), 0, (struct sockaddr *)&serv_addr, &addr_len);
		if (numbytes>0)
		{
			sprintf(ipaddr,"%s",(char*)inet_ntoa(serv_addr.sin_addr));
			//printf("Got %d bytes from %s\n",numbytes,ipaddr); fflush(stdout);
			switch (gdprs->ptype)
			{
				case YAFDP_TYPE_DISCOVERY_REPLY_SERVICE :				// We are only interested in these
					pdots(printdots);

					if (gdprs->request_handle==randomnumber)			// Is it a reply to our request ?
					{
						founddup=FALSE;
						for (i=0;i<dc;i++)					// For every device in list so far
						{
							if (strcmp(service_list[i].ipaddr,ipaddr)==0)	// Found this IP in the list?
								founddup=TRUE;
						}
						if (founddup!=TRUE)
						{
							service_list[dc].request_handle=gdprs->request_handle;
							strcpy(service_list[dc].ipaddr,ipaddr);
							service_list[dc].tcp_port=gdprs->tcp_port;
							service_list[dc].udp_port=gdprs->udp_port;
							dc++;
							if (quick==TRUE)				// quick, return with the first reply
							{
								close(drxfd);
								return(dc);
							}
						}
					}
				break;
			}
		}
	}
	close(drxfd);
	return(dc);
}






int yafdp_probe_devices(struct yafdp_reply_device_list devices_list[], int dlmax, int device_discov_timeoutms, int printdots)
{
	int drxfd=-1;
	struct sockaddr_in serv_addr;
        int addr_len=sizeof(serv_addr);
	int i,p;
	int numbytes;
	int randomnumber=0;
	char ipaddr[32];
	int dc=0;											// number of device and service records received
	int founddup=FALSE;										// Found duplicate
	int s=0;


	// Replies, allocate a buffer, place two structures ontop of that buffer
	char rbuffer[8192];
	struct yafdp_reply_device *gdprd=(struct yafdp_reply_device*)rbuffer;
	//struct yafdp_reply_service *gdprs=(struct yafdp_reply_service*)rbuffer;


	// Same as reply_device structure but with an extra field to store the IP address
	//struct yafdp_reply_device_list	devices_list[YAFDP_MAX_LIST_DEVICES];


	// Clear all the list structures
	for (i=0;i<dlmax;i++)
		bzero(&devices_list[i],sizeof(struct yafdp_reply_device_list));

	// Send device discovery request
	randomnumber=getnonzerorand();
	drxfd=yafdp_setup_receive_socket(TRUE, YAFDP_DISCOVRY_PORT, TRUE, FALSE);				// Prepare to receive replies from devices
	yafdp_send_discovery_request_devices(randomnumber);


	// Listen for replies to our discovery requests
	dc=0;
	for (p=0;p<device_discov_timeoutms;p++)
	{
		usleep(1000);										// one millisecond
		s++;
		if (s>200)
		{
			yafdp_send_discovery_request_devices(randomnumber);
			s=0;
		}

		bzero(rbuffer,sizeof(rbuffer));
		numbytes = recvfrom (drxfd, &rbuffer, sizeof(rbuffer), 0, (struct sockaddr *)&serv_addr, (socklen_t*)&addr_len);
		if (numbytes>0)
		{
			//printf("got %d bytes\n",numbytes);
			if (strcmp(gdprd->magic,YAFDP_MAGIC)==0)
			{
				sprintf(ipaddr,"%s",(char*)inet_ntoa(serv_addr.sin_addr));
				switch (gdprd->ptype)
				{
					case YAFDP_TYPE_DISCOVERY_REPLY_DEVICE :
						pdots(printdots);
						founddup=FALSE;
						for (i=0;i<dc;i++)					// For every device in list so far
						{
							//printf("devices_list[i].ipaddr = %s\n",devices_list[i].ipaddr);
							if (strcmp(devices_list[i].ipaddr,ipaddr)==0)	// Found this IP in the list
								founddup=TRUE;
						}

						if (founddup!=TRUE)					// if not a duplcate then copy into list
						{
							//printf("adding %s to line %d\n\n",ipaddr,dc);
							strcpy(devices_list[dc].ipaddr,ipaddr);
							devices_list[dc].request_handle = gdprd->request_handle;
							devices_list[dc].number_of_services = gdprd->number_of_services;
							strncpy(devices_list[dc].device_manufacturer, gdprd->device_manufacturer, sizeof(gdprd->device_manufacturer));
							strncpy(devices_list[dc].device_modelname, gdprd->device_modelname, sizeof(gdprd->device_modelname));
							strncpy(devices_list[dc].device_description, gdprd->device_description, sizeof(gdprd->device_description));
							strncpy(devices_list[dc].device_location, gdprd->device_location, sizeof(gdprd->device_location));
							dc++;
						}
					break;
				}
			}
		}
	}
	close(drxfd);
	return(dc);
}



// passed a single IP, the number of services that IP has in its list, return a list of services 
int yafdp_probe_service(int service_discov_timeoutms,char *ipaddr, uint16_t list_sequence, struct yafdp_reply_service *rs)
{
	int drxfd=-1;
	struct sockaddr_in serv_addr;
        unsigned int addr_len=sizeof(serv_addr);
	int t=0;
	int p=0;
	int foundserv=FALSE;
	int randomnumber=0;
	int numbytes=0;

	char rbuffer[8192];
	//struct yafdp_reply_device *gdprd=(struct yafdp_reply_device*)rbuffer;
	struct yafdp_reply_service *gdprs=(struct yafdp_reply_service*)rbuffer;

	drxfd=yafdp_setup_receive_socket(TRUE, YAFDP_DISCOVRY_PORT, TRUE, FALSE);				// Prepare to receive replies from devices
	if (drxfd<0)
		return(-1);

	t=0;
	p=0;
	foundserv=FALSE;
	randomnumber=getnonzerorand();
	yafdp_send_discovery_request_services(ipaddr,list_sequence,randomnumber);			// Ask device for this line from service table
	do
	{
		usleep(10000);										// ten millisecond
		p++;
		if (p>25)
			yafdp_send_discovery_request_services(ipaddr,list_sequence,randomnumber);	// Ask device for this line from service table
		bzero(rbuffer,sizeof(rbuffer));
		numbytes = recvfrom (drxfd, &rbuffer, sizeof(rbuffer), 0, (struct sockaddr *)&serv_addr, (socklen_t*)&addr_len);
		if (numbytes>0)
		{
			// Only process a reply that exactly matches our request
			if (strcmp(gdprs->magic,YAFDP_MAGIC)==0 && 
				   gdprs->ptype==YAFDP_TYPE_DISCOVERY_REPLY_SERVICE && 
				   gdprs->list_sequence == list_sequence &&
				   gdprs->request_handle == randomnumber )
			{
				foundserv=TRUE;
				memcpy(rs,gdprs,sizeof(struct yafdp_reply_service));
			}
		}
	} while (t<service_discov_timeoutms && foundserv!=TRUE);
	close(drxfd);											// Prepare to receive replies from devices
	return(foundserv);
}




#define xstr(s) str(s)
#define str(s) #s
#define ARP_CACHE       "/proc/net/arp"
#define ARP_STRING_LEN  1023
#define ARP_BUFFER_LEN  (ARP_STRING_LEN + 1)

/* Format for fscanf() to read the 1st, 4th, and 6th space-delimited fields */
#define ARP_LINE_FORMAT "%" xstr(ARP_STRING_LEN) "s %*s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s"

// For a given IP address lookup the MAC address from arp cache
// both input and output are ASCII text rather than byte arrays
int mac_from_ip (char *ipaddr, char* macaddr)
{
    char ipAddr[ARP_BUFFER_LEN];
    char hwAddr[ARP_BUFFER_LEN];
    char device[ARP_BUFFER_LEN];
    char header[ARP_BUFFER_LEN];
    FILE *arpCache = fopen(ARP_CACHE, "r");
    if (!arpCache)
    {
        perror("Arp Cache: Failed to open file \"" ARP_CACHE "\"");
        return 1;
    }

    /* Ignore the first line, which contains the header */
    if (!fgets(header, sizeof(header), arpCache))
    {
        return 1;
    }

    strcpy(macaddr,"NOT FOUND");
    while (3 == fscanf(arpCache, ARP_LINE_FORMAT, ipAddr, hwAddr, device))
    {
        //printf("%03d: Mac Address of [%s] on [%s] is \"%s\"\n",
                //++count, ipAddr, device, hwAddr);
	if (strcmp(ipAddr,ipaddr)==0)									// found it
		memcpy(macaddr,&hwAddr,18);
    }
    fclose(arpCache);
    return 0;
}


