
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "jcamsys.h"
#include "jcamsys_client_settings.h"


void client_settings_defaults(struct jcamsys_client_settings* s)
{
	bzero(s,sizeof(struct jcamsys_client_settings));
	sprintf(s->etc_path,"/etc/jcamsys");
	//s->quit			= FALSE;
	//s->archiver		= FALSE;
        //s->image_encryption     = TRUE;
        //s->daemon               = FALSE;
        sprintf(s->filename_passwd,"%s/jcam_passwd",s->etc_path);
        s->server_tcp_port      = JC_DEFAULT_TCP_PORT;
	s->persistent_connect	= TRUE;
	s->discover_server	= TRUE;
	sprintf(s->filename_shared_key,"%s/jcam_shared_key",s->etc_path);
	//s->ipbar_max_fails	= JC_MAX_IPBAR_FAILS;
	//s->kx_service		= TRUE;
	sprintf(s->filename_client_registration,"%s/jcam_registration",s->etc_path);
}



