// jcamsys_sharedmem.h


#include <stdatomic.h>


//jcsm->ci = camera information
//jcsm->cs = camera settings

struct sharedmem
{
	unsigned char 	quit;
	uint16_t	update_rate_hz;

	// semephore - one per camera image
	atomic_int			cdata_wlock[JC_MAX_IMAGES][JC_MAX_CAMS+1];

	// Camera images
	struct jcamsys_image		ci[JC_MAX_IMAGES][JC_MAX_CAMS+1];


	// Sensor state, includes when images are updated
	char		temp_value_changed;
	char		active_temps[JC_MAX_SENSORS+1];
	float		temps[JC_MAX_SENSORS+1];


	// ipbar
	struct ipbarlist iplist[IPBAR_TABLE_LEN];

	// server settings, including some current 'state' information
	uint16_t se_changed;
	struct jcamsys_server_settings	se;

	// Statistics
	struct jcamsys_statistics	st;

	// Sensors and cameras
	struct jcamsys_sensorstate	senss;

	// server state
	struct jcamsys_serverstate	ss;

	// Settings 'per camera' including the camera number assigned
	uint16_t cs_changed[JC_MAX_CAMS+1];		// poll_camera_settings_change
	struct jcamsys_camerasettings	cs[JC_MAX_CAMS+1];
	//char   camera_ips[JC_MAX_CAMS+1][20]; // TODO ...

	char archive_comment[JC_COMMENT_LEN];
	uint16_t archive_comment_changed;
};




// Prototypes
void jc_sm_clearlocks(struct sharedmem* sm);
void jc_sm_clearlocks(struct sharedmem* sm);
void jc_sm_release_cdata_lock(struct sharedmem* sm,  atomic_int p, int img, int cam);
void jc_sm_get_cdata_lock(struct sharedmem* sm, atomic_int p, int img, int cam);

