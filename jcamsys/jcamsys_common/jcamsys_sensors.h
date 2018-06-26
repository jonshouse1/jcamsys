// jcamsys_sensors.h

#include "jcamsys.h"

//#define JC_MAX_CAMS                     16
//#define JC_MAX_SENSORS                  32


// for use with 'senstype' value, array indexes so in order 0..max 
#define JC_MAX_SENSTYPE				4
#define JC_SENSTYPE_CAMERA			0
#define JC_SENSTYPE_MOTION			1
#define JC_SENSTYPE_TEMP			2
#define JC_SENSTYPE_MOISTURE			3


// Sensors, cameras are just another sensor
// active cameras are stored in sensor_active[0][camera] - it is MAX_SENSTYPE long
// but will be clipped at JC_MAX_CAMS+1 when sent over the network

// NOTE: used as inernal state but also formatted as a network message 

struct __attribute__((packed)) jcamsys_sensorstate
{
	// We can send this entire struct as a message of type JC_MSG_SENS_EVERYTHING
	unsigned char	magic[2];
	uint32_t	msglen;
	uint16_t	msgtype;                                // JC_MSG_SENS_EVERYTHING
	uint16_t	reqid;

	char		initialised;					// TRUE if initialised
	// incremented whan sensor_acive[senstype][sensor] modified
	uint16_t	sensor_active_changed[JC_MAX_SENSTYPE];
	char		sensor_active[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];

	// Time sensor values where modified
	uint32_t	sensor_change_time_ms[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];	
	uint32_t	sensor_change_ptime_ms[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];	// previous value	
	char		sensor_change_datetime[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1][32];	// asctime
	char		sensor_change_pdatetime[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1][32];	// previous

	// Sensor values, some use int, some float, some char but just store it as
	// a big mass to make moving it around simple
	int16_t		sensor_value_changed[JC_MAX_SENSTYPE];
	float		sensor_fvalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
	uint16_t	sensor_ivalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
	char		sensor_cvalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1][16];

	// Sensor information
	// Change rate:
	// 	for cameras 250ms would be 1000/250 frames per second
	// 	slower sensors may have multi second values
	// 	if not imprtant then value will be left at zero
	int16_t		sensor_change_rate_ms[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
	char		sens_lastbyte;					// for structure size wrangling
};



