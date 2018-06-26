
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
#include "jcamsys_settings.h"


void settings_defaults(struct jcamsys_settings* s)
{
	bzero(s,sizeof(struct jcamsys_settings));
	sprintf(s->etc_path,"/etc/jcamsys");
	s->quit			= FALSE;
	s->archiver		= FALSE;
        s->image_encryption     = TRUE;
        s->daemon               = FALSE;
        sprintf(s->filename_passwd,"%s/jcam_passwd",s->etc_path);
        s->server_tcp_port      = JC_DEFAULT_TCP_PORT;
	s->persistent_connect	= TRUE;
	s->discover_server	= TRUE;
	sprintf(s->filename_site_key,"%s/jcam_site_key",s->etc_path);
	s->ipbar_max_fails	= JC_MAX_IPBAR_FAILS;
	s->camera_fps		= JC_DEF_CAMERA_FPS;
	s->kx_service		= TRUE;
}



