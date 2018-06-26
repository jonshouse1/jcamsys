// jcamsys_network.c


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_network.h"
#include "jcamsys_cipher.h"
#include "ipbar.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"

#include "yet_another_functional_discovery_protocol.c"

int is_valid_fd(int fd)
{
    return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}


int jc_hostname_to_ip(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
         
	if ( (he = gethostbyname( hostname ) ) == NULL) 
	{
        	// get the host info
        	herror("gethostbyname");
        	return(JC_ERR);
	}
 
	addr_list = (struct in_addr **) he->h_addr_list;

	for(i = 0; addr_list[i] != NULL; i++) 
	{
        	//Return the first one;
        	strcpy(ip , inet_ntoa(*addr_list[i]) );
		return(JC_OK);
    	}
	return(JC_OK);
}





// For clients, must call only once though.
int create_listening_tcp_socket(int port)
{
	static int sfd=-1;
	static struct sockaddr_in server;

	bzero(&server, sizeof(struct sockaddr_in));
	sfd=socket(AF_INET , SOCK_STREAM , 0);
	if (sfd==-1)
		return(JC_ERR);

        //Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);

	int reuse = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEPORT) failed");
#endif

	if( bind(sfd,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("bind failed. Error");
		exit(1);
	}

        // Listen
	listen(sfd , 5);
	return(sfd);
}




// Returns file descriptor or negative value on error
// Note - as we store sockaddr_in locally we can only use this for one connection
int jc_connect_to_server(char *ipaddr, int tcp_port, int blocking)
{
	int sockfd = 0;
	static struct sockaddr_in serv_addr; 
	int val=0;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	{
        	printf("\n Error : Could not create socket \n");
        	return (JC_ERR);
    	} 

    	memset(&serv_addr, '0', sizeof(serv_addr)); 
    	serv_addr.sin_family = AF_INET;
    	serv_addr.sin_port = htons(tcp_port); 

    	if(inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr)<=0)
    	{
        	printf("\n inet_pton error occured\n");
        	return(JC_ERR);
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		close(sockfd);
		//printf("\n Error : Connect Failed \n");
        	return(JC_ERR);
	}

	if (blocking!=TRUE)
	{
		val=fcntl (sockfd, F_GETFL, 0);
        	fcntl (sockfd, F_SETFL, val | O_NONBLOCK);
	}

	// TCP_NODELAY will cause TCP to generate a packet with each write.
	// must avoid writing single bytes if this is on.
	int i = 1;
	setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
	return(sockfd);
}


int jc_fd_blocking(int sockfd,int torf)
{
	int val=0;
	int x=0;

	val=fcntl(sockfd, F_GETFL, 0);
	if (torf==TRUE)	
		x=fcntl(sockfd, F_SETFL, val & ~O_NONBLOCK);		// is blocking
	else	x=fcntl(sockfd, F_SETFL, val | O_NONBLOCK);		// is non blocking
	return(x);
}




// Returns >=0  number of services found
// -2 bind failed (busy, sleep and retry)
// -1 error
int jc_discover_server(char *ipaddr, int *port, int silent, int includelocal)
{
        int n=0;
        int i=0;
        char service_protocol_name_short[64];
        struct yafdp_reply_service_list service_list[YAFDP_MAX_SERVICE_LIST];
	//char    text[8192];
	//char	cmd[1024];

	if (includelocal==TRUE)
	{
		// server on this machine? 
		n=proc_find("jcamsysserver");

		// What socket is the server listening on
		//if (n>0)
		//{
			//printf("pid=%d\t",n);  fflush(stdout);
			//sprintf(cmd,"netstat -npl 2>/dev/null |grep 'jcamsysserver' |grep 'tcp' |cut -d':' -f2 |cut -d' ' -f1");
			//if (run_command(cmd,(char*)&text,sizeof(text))!=JC_ERR)
			//{
				//*port=atoi(text);
				//if (*port<=0)				// cant read it
					//*port=JC_DEFAULT_TCP_PORT;	// guess
				//strcpy(ipaddr,"127.0.0.1");
				//return(1);
			//}
		//}

		// Server now has more than one listening TCP socket, so just
		// assume the default port is in use.  
		// Can always use -host -p instead of discovery
		if (n>0)
		{
			printf("**this machine, port guessed** ");
			fflush(stdout);
			*port=JC_DEFAULT_TCP_PORT;
			strcpy(ipaddr,"127.0.0.1");
			return(1);
		}
	}


	// Not running locally, so use UDP service to discover server
        strcpy(service_protocol_name_short,"jcamsysserver");
        bzero(&service_list,sizeof(service_list));
        n=yafdp_discover_service((struct yafdp_reply_service_list*)&service_list[0], service_protocol_name_short, 3000, FALSE, TRUE, TRUE);
        if (n>0)
        {
              	strcpy(ipaddr,service_list[i].ipaddr);
              	*port=service_list[i].tcp_port;
        }
        else	ipaddr[0]=0;
	return(n);
}




//https://lkml.org/lkml/2002/6/10/68
int selectcheck(int s)
{
        fd_set rset, wset, xset;
        struct timeval timeout;
        int ret;

        FD_ZERO(&rset);
        FD_SET(s, &rset);
        wset = xset = rset;

        timeout.tv_sec  = 0;
        timeout.tv_usec         = 0;

        errno = 0;
        ret = select(s + 1, &rset, &wset, &xset, &timeout);
        if (FD_ISSET(s,&rset)) 
	{
            fprintf(stderr, "  socket is readable\n");
	}
        if (FD_ISSET(s,&wset)) 
	{
            fprintf(stderr, "  socket is writeable\n");
	}
        if (FD_ISSET(s,&xset)) 
	{
            fprintf(stderr, "  socket has an exception\n");
	}

        return ret;
}



// How many bytes are available to read from socket
// Retuns >1 (1 more more bytes)
//         0 sucess ioctl, but no bytes available yet
// 	  -1 JC_ERR (socket has closed)
int jc_peekbytes(int sockfd)
{
        int res=0;
        int bytesavailable=0;
	unsigned char b;

    	ssize_t x = recv(sockfd, &b, 1, MSG_PEEK);
	//printf("sockfd=%d x=%d\n",sockfd,x); fflush(stdout);
	if (x==0)
		return(JC_ERR);

	// possible IOCTL retuns 0=sucess, EBADF(9), EFAULT(14), EINVAL(22), ENOTTY(25)
        res=ioctl(sockfd, FIONREAD, &bytesavailable);
        if (res==0)					// ioctl worked
        	return(bytesavailable);			// tell caller N bytes
	return(JC_ERR);					// res=E something, error
}



