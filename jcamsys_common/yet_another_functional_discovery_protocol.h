// Generic discovery protocol for devices or services
//
//	protocol version (pvar)		0.1
//	J.Andrews, 12 May 2017
//
// (O)  Optional, may be blank or zero field
// (UC) Uppser Case
// (MC) Mixed case
// (LC) Lower case
// (ZT) is zero terminated string


//#define YAFDP_ALL_REPLIES_BROADCAST						// uncomment to send all responses as UDP broadcast

#define YAFDP_PVER_MAJ					0			// Packet format is version 0.1
#define YAFDP_PVER_MINOR				1
#define YAFDP_DISCOVRY_PORT				8118
#define YAFDP_BCAST_ADDR				"255.255.255.255"
#define YAFDP_MAGIC 					"YAFDP"
#define YAFDP_MAGIC_SIZE				6
#define YAFDP_TYPE_DISCOVERY_REQUEST_DEVICES		'D'
#define YAFDP_TYPE_DISCOVERY_REQUEST_SERVICE		'?'
#define YAFDP_TYPE_DISCOVERY_REQUEST_SERVICES		'S'
#define YAFDP_TYPE_DISCOVERY_REPLY_DEVICE		'd'			// Replying with a yafdp_reply_device structure
#define YAFDP_TYPE_DISCOVERY_REPLY_SERVICE		's'			// Replying with a yafdp_reply_service structure


// List of short names for serices I have written
#define YAFDP_SERVICE_PROTOCOL_NAME_JLC			"JLC"			// Jons lighting controller (master)
#define YAFDP_SERVICE_PROTOCOL_NAME_JLD			"JLD"			// Jons lighting device (fixture)
#define YAFDP_SERVICE_PROTOCOL_NAME_JLB			"JLB"			// Jons lighting bridge (universe to physical)

#define YAFDP_MAX_SERVICES				100
#define YAFDP_MAX_SERVICE_LIST				100
#define YAFDP_MAX_LIST_DEVICES				1000
#define YAFDP_MAX_LIST_SERVICES				10000



// Request a reply from a given host (IP address)
// First 4 feilds of each structure should be the same
struct __attribute__((packed)) yafdp_request_devices
{
	char 		magic[YAFDP_MAGIC_SIZE];
	char		pver[2];						// Protocol version uint8 twice
	char		ptype;
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests
};

struct __attribute__((packed)) yafdp_request_services
{
	char 		magic[YAFDP_MAGIC_SIZE];
	char		pver[2];						// Protocol version uint8 twice
	char		ptype;
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests
	uint16_t	list_sequence;						// 0=Send the whole list now, value=send just this item
};

struct __attribute__((packed)) yafdp_request_service
{
	char 		magic[YAFDP_MAGIC_SIZE];
	char		pver[2];						// Protocol version uint8 twice
	char		ptype;
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests
	char		service_protocol_name_short[33];			// See examples (lc,ZT)
};


// Reply contains one of these
struct __attribute__((packed)) yafdp_reply_device
{
	char 		magic[YAFDP_MAGIC_SIZE];
	char		pver[2];						// Protocol version uint8 twice
	char		ptype;
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests

	uint16_t	number_of_services;					// Number of services in the list of services, 0=Dont support service lists
	char 		device_manufacturer[17];				// Manufacturer of physical device (UC)
	char		device_modelname[33];					// (O,MC,ZT)
	char		device_description[33];					// How the user describes the device itself (O,MC,ZT)
	char		device_location[33];					// How the user describes the device location (O,MC,ZT)
	//char		u1c[2];
	//char		u1s[17];
};


// yafdp_similar to reply_device plus an extra field to store the IP address of the station sending the reply
struct __attribute__((packed)) yafdp_reply_device_list
{
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests
	uint16_t	number_of_services;					// Number of services in the list of services, 0=Dont support service lists
	char 		device_manufacturer[17];				// Manufacturer of physical device (UC)
	char		device_modelname[33];					// (O,MC,ZT)
	char		device_description[33];					// How the user describes the device itself (O,MC,ZT)
	char		device_location[33];					// How the user describes the device location (O,MC,ZT)
	char 		ipaddr[16];
};




struct __attribute__((packed)) yafdp_reply_service
{
	char 		magic[YAFDP_MAGIC_SIZE];
	char		pver[2];						// Protocol version uint8 twice
	char		ptype;
	uint16_t	request_handle;						// Simple mechanism to ignore duplicate requests
	uint16_t	list_sequence;						// Allows receivers building lists to know they are missing one

	uint16_t	udp_port;						// This services primary UDP port (O) 0=unknown or N/A
	uint16_t	tcp_port;						// This services primary TCP port (O) 0=unknown or N/A
	// service_protocol_name is used for identification, it must be exact and consistent 
	char		service_protocol_name_short[33];			// See examples (LC,ZT)
	char		u1[17];							// User supplied field (MC,ZT)
	char		u2[17];							// User supplied field (MC,ZT)
};


struct __attribute__((packed)) yafdp_reply_service_list
{
	uint16_t	request_handle;
	uint16_t	udp_port;
	uint16_t	tcp_port;
	char 		ipaddr[16];
};



// Prototypes
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);
int yafdp_setup_receive_socket(int listen, int port, int silent, int dontexit);

