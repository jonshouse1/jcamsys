// jcamsys_settings.h

#include <stdint.h>

struct __attribute__((packed)) jcamsys_server_settings
{
	// Puase and resume server child processes (if loaded)
        char            enable_archiver;		// T=resume archiving, F=suspend
        char            enable_keyexchange;		// T=resume, F=suspend
	char		enable_http;			// T=resume, F=suspend

	// These are command line options, some stop a server function from loading
	char		nodiscover;			// T=Dont yafdp discover servers
	char		nokx;				// T=No KX server started
	char		noar;				// T=No archiver started
	char		nohttp;				// T=No http server started


        char            exit_on_archive_error;		// T=Exit server on file write error


        // True if currently archiving images, false to suspend archiving 
        char            archive_verbose;		// T=Archive console text output
        uint16_t        archive_rate_ms;		// Rate to archive images (normal)
        uint16_t        sarchive_rate_ms;		// Rate when sensor has triggered
        uint16_t        archive_image_size;     	// 0=largest etc
        char            archive_path[2048];


	char		daemon;				// T=running detached from console
	char		etc_path[1024];
	char		filename_passwd[1024];
	char		filename_shared_key[1024];
	char		filename_client_registration[1024];


	uint16_t	server_tcp_port;
	uint16_t	http_server_tcp_port;
	uint16_t	kx_server_tcp_port;
	uint32_t	kx_timeoutms;
	uint16_t	ipbar_max_fails;

	// State is server side but action is done client side
	char		samerescams;			// same resolution on all cameras
	char		samedatecams;			// same date on all cameras
	char		docrc;				// T=CRC check images
};

void settings_defaults(struct jcamsys_server_settings* s);
