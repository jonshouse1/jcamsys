// jcamsys_protocol.h
// Version 0.2
// 26 Mar 2018
//
// The server may send small messages at any point, the client must accept them though it choose
// to discard them.
// server messages may indicate a camera image has been updated, but it up to the client to
// request the new image.
//
// sensor value changes are sent by the server, the client can choose to request
// reqid is the clients request id (incrimenting number), replies to requests also contain this
// as a 'reference' 
//
// Cameras are just another 'sensor', simplifies design

#include <stdint.h>
#include "jcamsys.h"

#define JC_PROTOCOL_VERSION	1

// Return codes from the protocol interpreter
#define JC_INT_IDLE							// The interpreter is awaiting a message
#define JC_INT_PARTIAL							// got something


#define JC_MAX_HEADERLEN				64		// Maximum likely size of the header
#define JC_MAX_MSGLEN					JC_MAX_HEADERLEN + JC_MAX_COMP_IMAGESIZE

// Note this is 'dirty' in network programming terms, we assume both the sender and receiver are
// the same endian (or able to pretend so).  If the server is not sending a MSG response to any
// specific request then reqid=0

// Messages start with this
#define MAGIC0	'J'
#define MAGIC1	'C'
#define MAGIC2	'A'
#define MAGIC3	'M'

// Actions (for statistics code
#define JC_AC_NONE					0
#define JC_AC_REGISTER					1
#define JC_AC_REQ_IMAGE					3
#define JC_AC_IMAGE					4
#define JC_AC_MESSAGEMASK				5
#define JC_AC_CAMERASETTINGS				6
#define JC_AC_SERVERSETTINGS				7
#define JC_AC_SENS					8

// Requests, normally from client to server
#define JC_REQ_TIMEMS					10
#define JC_REQ_SENS_ACTIVE				11
#define JC_REQ_SENS_VALUES				12		// single value
#define JC_REQ_SENS_VALUE				13		// single value
#define JC_REQ_SENS_EVERYTHING				14		// sensorstate struct
#define JC_REQ_STATS					15
#define JC_REQ_CAMERASETTINGS				16
#define JC_REQ_SERVERSETTINGS				18
#define JC_REQ_COMMENT					19
#define JC_REQ_IMAGE					20

// messages, often, but not always, replies to requests
#define JC_MSG_NONE					0
#define JC_MSG_TIMEMS					100
#define JC_MSG_MESSAGEMASK				102
#define JC_MSG_REGISTER					103
#define JC_MSG_SENS_ACTIVE				104
#define JC_MSG_SENS_VALUES				105
#define JC_MSG_SENS_VALUE				106
#define JC_MSG_SENS_EVERYTHING				107		// sensorstate struct
#define JC_MSG_FRAME_NUMBER				108	
#define JC_MSG_CAMERASETTINGS				115		// jcamsys_camerasettings.h
#define JC_MSG_DTS					110		// Datetime string
#define JC_MSG_STATS					114
#define JC_MSG_SERVERSETTINGS				118
#define JC_MSG_COMMENT					119
#define JC_MSG_SERVERSTATE				120
#define JC_MSG_IMAGE					200		// normally jpeg


// reqid is either "the next number in a sequence" if making a request or the return of that number for 
// a response.
// For example:  a message of type 'JC_REQ_IMAGE' sent to the server with a 'reqid=20' will generate a  
// 'JC_MSG_IMAGE' response the the same reqid.
// on some transactions it does not matter, on others it indicates a specific response to a specific 
// request.
struct __attribute__((packed)) jcamsys_header
{
	char			magic[4];
	uint16_t		msgtype;				// JC_REQ__... or JC_MSG_...
	uint16_t		reqid;
	uint64_t		sendtimems;		
	uint32_t		msglen;
};



// what sensors are active for a given 'senstype'
struct __attribute__((packed)) jcamsys_req_sens_active
{
	uint16_t		senstype;				// one of JC_SENSTYPE_....
};


struct __attribute__((packed)) jcamsys_req_sens_values
{
	uint16_t		senstype;
};


struct __attribute__((packed)) jcamsys_req_image
{
	uint16_t		cam;					// camera
	uint16_t		img;					// IFULL, IPREV etc
};


struct __attribute__((packed)) jcamsys_req_stats
{
	char 			dummy;
};


struct __attribute__((packed)) jcamsys_req_camerasettings
{
	uint16_t		can;
};



// not all fields are applicable to all modes.
#define JC_ROLE_VIEWER					1
#define JC_ROLE_CAMERA					2
#define JC_ROLE_CONSOLE					3
struct __attribute__((packed)) jcamsys_register
{
	char			id[JC_IDLEN];				// ID is used as a handle to push saved settings
	char			role;
};



// used with 'jcamsys_msg_frame_number' for feeders to stay realtime. and viewers
// to keep syncronised with image updates from feeders
struct __attribute__((packed)) jcamsys_messagemask
{
	char			frame_changes[JC_MAX_IMAGES][JC_MAX_CAMS+1];
	char			camera_settings[JC_MAX_CAMS+1];
	char			sensors;	// be corse at the moment
	char			dts;		// server should send datetime text
	char			imagesizes;	// send MSG_STATS if image sizes change
	char			server_settings;// notify when server settings change
	char			comment;	// Text line info from server
};


// Both an image and a message
struct __attribute__((packed, aligned(4))) jcamsys_image
{
	char 			sipaddr[16];				// IP address of the source (originator)
	char			sid[JC_IDLEN];				// ID of the originator

	uint16_t		img;					// IFULL, IPREV etc
	uint16_t		cam;
	char			quality;

	uint16_t		image_type;				// at the moment only JC_IMAGE_TYPE_JPEG
	char			crypted;				// TRUE if cbytes holds encrypted data
	uint32_t		optional_crc32;				// 0 if unused
	uint32_t		cbytes;
	uint16_t		width;	
	uint16_t		height;
	uint16_t		frame_number;
	uint16_t		update_rate_ms; 			// helps with playback later
	uint64_t		timestamp;				// should be time including ms
	char			cdata[JC_MAX_MSGLEN-JC_MAX_HEADERLEN];	// compressed image data
};



struct __attribute__((packed)) jcamsys_sens_active
{
	uint16_t		senstype;
	// sensactive must be the last member of the struct as a short version may be sent
	char			sensor_active[JC_MAX_SENSORS+1];
};


// All values for one senstype
struct __attribute__((packed)) jcamsys_sens_values
{
	uint16_t		senstype;
	uint16_t		sensor;

	float			sensor_fvalue[JC_MAX_SENSORS+1];
	uint16_t		sensor_ivalue[JC_MAX_SENSORS+1];
	char			sensor_cvalue[JC_MAX_SENSORS+1][16];
};

// A single sensor value
struct __attribute__((packed)) jcamsys_sens_value
{
	uint16_t		senstype;
	uint16_t		sensor;

	float			sensor_fvalue;
	uint16_t		sensor_ivalue;
	char			sensor_cvalue[16];
};

// Used for many thing, including syncronisation on viewers and bandwidth saturation
// on feeders.
struct __attribute__((packed)) jcamsys_frame_number
{
	uint16_t		img;
	uint16_t		cam;
	uint16_t		frame_number;
};


// Date time string, used by cameras to overlay datetime text on images
struct __attribute__((packed)) jcamsys_dts
{
	char			dts[32];		// think its 29 + \0=30
};


// The header contains the senders time in ms, just add a minimum payload
struct __attribute__((packed)) jcamsys_timems
{
	char			t;
};



struct __attribute__((packed)) jcamsys_serverstate
{
        char            archiving_ok;                   // T=writing images to archive ok
        char            kx_active;                      // T=server ready
        char            http_active;                    // T=server ready
};



#define JC_CMNT_GENRAL		0
#define JC_CMNT_SERVER		1
#define JC_CMNT_ARCHIVE		2
struct __attribute__((packed)) jcamsys_comment
{
	uint16_t		server;
	char			comment[JC_COMMENT_LEN];
};


// Prototypes for jcamsys_protocol.c
void print_timems(uint64_t tms);
void print_struct_jcamsys_image(struct jcamsys_image* jci);
void print_struct_statistics(struct jcamsys_statistics* st);
int valid_image_cam(int img, int cam, char *pn, char *ipaddr, int silent);
void printmm(struct jcamsys_messagemask* mx);
int jc_sockwrite(int fd, unsigned char* buf, int len);
int jcam_read_and_process_message(int sockfd, struct jcamsys_key* key, struct jcamsys_sensorstate* sen, unsigned char*buf, int bufsize, int deb);
int jcam_read_message(int sockfd, struct jcamsys_key* key, unsigned char*buf, int bufsize, int *msgtype, uint16_t *reqid, uint64_t *sendtimems, int timeoutms);

int jc_send_header(int sockfd, int msgtype, int msglen, uint16_t reqid);
int jc_request_stats(int sockfd, uint16_t reqid);
int jc_request_image(struct jcamsys_key* key, int fd, uint16_t reqid, int img, int cam);
int jc_request_server_settings(int sockfd, uint16_t reqid);
int jc_msg_image(struct jcamsys_key* key, struct jcamsys_image* jci, int sockfd, int docrc, uint16_t reqid);
int jc_msg_sens_active(struct jcamsys_key* key, struct jcamsys_sensorstate* ss, int fd, uint16_t reqid, int senstype);
int jc_msg_sens_values(struct jcamsys_key* key, struct jcamsys_sensorstate* ss, int sockfd, uint16_t reqid, int senstype);
int jc_msg_sens_value(struct jcamsys_key* key, int sockfd, int16_t reqid, int senstype, float fvalue, int ivalue, char* cvalue);
int jc_msg_frame_number(struct jcamsys_key* key, uint16_t fn, int sockfd, int img, int cam);
int jc_msg_messagemask(struct jcamsys_key* key, int sockfd, uint16_t reqid, struct jcamsys_messagemask* mm);
int jc_msg_dts(struct jcamsys_key* key, int sockfd, char *dts);
int jc_register_as_feeder(struct jcamsys_key* key, int sockfd, uint16_t reqid, int cam);
int jcamsys_img_headerlen( struct jcamsys_image* jci );
int jc_image_copy( struct jcamsys_image* jcid, struct jcamsys_image* jcis);

void jc_image_force_crypted(struct jcamsys_key* key, struct jcamsys_image* jci);
void jc_image_force_uncrypted(struct jcamsys_key* key, struct jcamsys_image* jci);
void jc_image_prepare(struct jcamsys_key* key, struct jcamsys_image* jci, int img, int cam, int image_type, int width, int height, int cbytes, char *sid, int frame_number);

int jc_msg_camerasettings(struct jcamsys_key* key, struct jcamsys_camerasettings* cs, int sockfd, uint16_t reqid);
int jc_msg_register(struct jcamsys_key* key, struct jcamsys_register* ri, int sockfd);
int jc_msg_timems(struct jcamsys_key* key, int sockfd, uint16_t reqid);

int jc_msg_stats(struct jcamsys_key* key, struct jcamsys_statistics* st, int sockfd, uint16_t reqid);
int jc_msg_serversettings(struct jcamsys_key* key, struct  jcamsys_server_settings* s, int sockfd, uint16_t reqid);
int jc_msg_comment(struct jcamsys_key* key, int server, char* c, int sockfd, uint16_t reqid);
int jc_msg_serverstate(struct jcamsys_key* key, struct jcamsys_serverstate* ss, int sockfd, int reqid);
