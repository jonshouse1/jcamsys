// jcamsys_settings.h

#include <stdint.h>

struct __attribute__((packed)) jcamsys_client_settings
{
	int		server_tcp_port;
	char		server_hostname[1024];		// may be blank
	char		server_ipaddr[32];		// ASCII ip
	int		discover_server;		// TRUE or FALSE
	int		persistent_connect;		// TRUE or FALSE
	//int		ipbar_max_fails;
	//char		kx_service;			// kx service possible

        char            etc_path[1024];
        char            filename_passwd[1024];
        char            filename_shared_key[1024];
        char            filename_client_registration[1024];
};

void client_settings_defaults(struct jcamsys_client_settings* s);
