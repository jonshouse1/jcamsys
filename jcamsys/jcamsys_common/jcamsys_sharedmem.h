// jcamsys_sharedmem.h


#include <stdatomic.h>


//jcsm->ci = camera information
//jcsm->cs = camera settings

struct sharedmem
{
	unsigned char 	quit;
	uint16_t	update_rate_hz;

	// At the moment width/height/fps is the same for all cameras
	uint16_t	gquality;
	uint16_t	gmode;		// default mode JC_MODE_640x480
	uint16_t	gwidth;
	uint16_t	gheight;
	uint16_t	gfps;
	char		gdocrc;		// T or F
	uint16_t	preview_divisor;


	// semephore - one per camera image
	atomic_int			cdata_wlock[JC_MAX_IMAGES][JC_MAX_CAMS+1];


	struct jcamsys_image		ci[JC_MAX_IMAGES][JC_MAX_CAMS+1];				// chunky struct including image data


	// Sensor state, includes when images are updated
	char		temp_value_changed;
	char		active_temps[JC_MAX_SENSORS+1];
	float		temps[JC_MAX_SENSORS+1];

	
	// control starting stopping child processes, TRUE enabled it
	char		enable_archiver;
	char		enable_keyexchange;
	char		archiving;


	// ipbar
	struct ipbarlist iplist[IPBAR_TABLE_LEN];


	// Sensors and cameras
	struct jcamsys_sensorstate	senss;

	// Settings 'per camera' including the camera number assigned
	struct jcamsys_camerasettings	cs[JC_MAX_CAMS+1];


	// Oldest image (in epoc milliseconds) for each camera.
	uint64_t	oldest_image[JC_MAX_CAMS+1];
};




// Prototypes
void jc_sm_clearlocks(struct sharedmem* sm);
void jc_sm_clearlocks(struct sharedmem* sm);
void jc_sm_release_cdata_lock(struct sharedmem* sm,  atomic_int p, int img, int cam);
void jc_sm_get_cdata_lock(struct sharedmem* sm, atomic_int p, int img, int cam);

