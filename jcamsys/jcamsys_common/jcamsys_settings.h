// jcamsys_settings.h

#include <stdint.h>

struct __attribute__((packed)) jcamsys_settings
{
	int 		quit;
	int		image_encryption;
	int		daemon;
	int		archiver;
	char		etc_path[2048];
	char		filename_passwd[2048];
	char		filename_site_key[2048];

	int		server_tcp_port;
	char		server_hostname[2048];		// may be blank
	char		server_ipaddr[32];		// ASCII ip
	int		discover_server;		// TRUE or FALSE
	int		persistent_connect;		// TRUE or FALSE
	int		ipbar_max_fails;
	int		camera_fps;
	char		kx_service;
};

void settings_defaults(struct jcamsys_settings* s);
