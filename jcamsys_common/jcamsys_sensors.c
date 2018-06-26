// jcamsys_sensors.c
// Everything sensor related

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
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_network.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_statistics.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
//#include "jcamsys_archiver.h"
//#include "jcamsys_keyexchange.h"
#include "jcamsys_images.h"
#include "jcamsys_sensors_prot.h"

//#include "id.h"

//struct __attribute__((packed)) jcamsys_sensorstate
//{
        // We can send this entire struct as a message of type JC_MSG_SENS_EVERYTHING
        //unsigned char   magic[2];
        //uint32_t        msglen;
        //uint16_t        msgtype;                                // JC_MSG_SENS_EVERYTHING
        //uint16_t        reqid;

        //char            initialised;                                    // TRUE if initialised
        // incremented whan sensor_acive[senstype][sensor] modified
        //uint16_t        sensor_active_changed[JC_MAX_SENSTYPE];
        //char            sensor_active[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];

        // Time sensor values where modified
        //uint32_t        sensor_change_time_ms[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
        //uint32_t        sensor_change_ptime_ms[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];      // previous value       

        // Sensor values, some use int, some float, some char but just store it as
        // a big mass to make moving it around simple
        //int16_t         sensor_value_changed[JC_MAX_SENSTYPE];
        //float           sensor_fvalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
        //uint16_t        sensor_ivalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1];
        //char            sensor_cvalue[JC_MAX_SENSTYPE][JC_MAX_SENSORS+1][16];

        //char            sens_lastbyte;                                  // for structure size wrangling
//};



int jc_sensor_valid(int senstype, int sensor)
{
	if ( (senstype <0) | (senstype >JC_MAX_SENSORS) )
	{
		printf("jc_sensor_valid() - illegal senstype value %d\n",senstype);
		fflush(stdout);
		return(JC_ERR);
	}
	if ( (sensor <0) | (sensor >JC_MAX_SENSORS) )
	{
		printf("jc_sensor_valid() - illegal sensor value %d\n",sensor);
		fflush(stdout);
		return(JC_ERR);
	}
	return(JC_OK);
}


void jc_sensor_mark_time(struct jcamsys_sensorstate* senss, int senstype, int sensor)
{
	senss->sensor_change_ptime_ms[senstype][sensor] = senss->sensor_change_time_ms[senstype][sensor];
	senss->sensor_change_time_ms[senstype][sensor]=current_timems();	
}



// Mark a sensor as becoming active, no effect if already marked active
void jc_sensor_mark_active(struct jcamsys_sensorstate* senss, int senstype, int sensor)
{
	if (jc_sensor_valid(senstype, sensor)!=JC_OK)
		return;
	if (senss->sensor_active[senstype][sensor]==TRUE)		// already active
		return;							// dont record event
	senss->sensor_active[senstype][sensor]=TRUE;
	jc_sensor_mark_time(senss, senstype, sensor);
	senss->sensor_active_changed[senstype]++;
}


void jc_sensor_mark_inactive(struct jcamsys_sensorstate* senss, int senstype, int sensor)
{
	if (jc_sensor_valid(senstype, sensor)!=JC_OK)
		return;
	if (senss->sensor_active[senstype][sensor]==FALSE)	// already deactive
		return;							// dont record event
	senss->sensor_active[senstype][sensor]=FALSE;
	jc_sensor_mark_time(senss, senstype, sensor);
	senss->sensor_active_changed[senstype]++;
}



// Got a new value fed in.
void jc_sensor_newvalue(struct jcamsys_sensorstate* senss, struct jcamsys_sens_value* msgav)
{
	if (jc_sensor_valid(msgav->senstype, msgav->sensor)!=JC_OK)
		return;

	if (senss->sensor_active[msgav->senstype][msgav->sensor]!=TRUE)
	{
		jc_sensor_mark_active(senss, msgav->senstype, msgav->sensor);
		jc_sensor_mark_time(senss, msgav->senstype, msgav->sensor);

		// Record the actual sensor value
		senss->sensor_fvalue[msgav->senstype][msgav->sensor] = msgav->sensor_fvalue;
		senss->sensor_ivalue[msgav->senstype][msgav->sensor] = msgav->sensor_ivalue;
		strcpy((char*)senss->sensor_cvalue[msgav->sensor], msgav->sensor_cvalue);
		
		// Mark that the value changed
		senss->sensor_value_changed[msgav->senstype]++;	
	}
}



