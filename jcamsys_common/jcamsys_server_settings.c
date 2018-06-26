
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
#include "jcamsys_server_settings.h"


void settings_defaults(struct jcamsys_server_settings* s)
{
	bzero(s,sizeof(struct jcamsys_server_settings));
	sprintf(s->etc_path,"/etc/jcamsys");
        s->daemon               = FALSE;
        sprintf(s->filename_passwd,"%s/jcam_passwd",s->etc_path);
	sprintf(s->filename_shared_key,"%s/jcam_shared_key",s->etc_path);
	sprintf(s->filename_client_registration,"%s/jcam_registration",s->etc_path);


	s->ipbar_max_fails	= JC_MAX_IPBAR_FAILS;
	s->server_tcp_port      = JC_DEFAULT_TCP_PORT;
	s->http_server_tcp_port = JC_DEFAULT_HTTP_TCP_PORT;
	s->kx_server_tcp_port	= JC_DEFAULT_KX_TCP_PORT;
	s->exit_on_archive_error= JC_ARCHIVE_ERROR_EXIT;

	s->kx_timeoutms		= 1000 * 60 * 15;	// 15 min
	s->archive_rate_ms	= 1000;
	s->sarchive_rate_ms	= 1000;

	// These suspend or resume running servers, initially assume suspended.
	// server settings loaded later may turn these on
	s->enable_archiver	= FALSE;
	s->enable_keyexchange	= FALSE;
	s->enable_http		= FALSE;


        // These are command line options, some stop a server function from loading
	s->nodiscover		= FALSE;
	s->nokx			= FALSE;
	s->noar			= FALSE; 
	s->nohttp		= FALSE;


	// Cameras
	s->samerescams		= TRUE;
	s->samedatecams		= TRUE;
	s->docrc		= TRUE;
}



