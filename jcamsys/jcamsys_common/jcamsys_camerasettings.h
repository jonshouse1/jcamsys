// jcamsys_camerasetting.h
// sent by the server to the camera upon registration
// 5 Apr  2018
//
//

#include <stdint.h>
//#include "jcamsys.h"

// This strcuture is stored in shared memory on the server and sent to the
// cameras when they register.
// The camera has no local settings other than ID - simplifies managing then
// centrally via the server and 'server console'
// This struct becomes 'JC_MSG_CAMERASETTINGS' when sent via jc_msg_camerasettings();


struct __attribute__((packed)) jcamsys_camerasettings
{
	// these feilds are really for use by the server,
	char	  changed;		// TRUE or FALSE
	uint16_t  image_type;		// only JC_IMAGE_TYPE_JPEG at the moment
	char	  camname[32];
	char	  id[32];

	uint16_t  mode;			// value from jcamsys_modes.h
	uint16_t  width;		// populated from 'mode' normally unless 
	uint16_t  height;		// mode is 'custom'

	uint16_t  cam;			// Camera number 1..JC_MAX_CAMS
	char	  fliph;		// T or F, T=Camera is upside down
	char	  flipv;		// T or F, T=Camera is mirrored left to right

        uint16_t  update_rate_ms;	// ms between each frame, 100=10Hz for example
	uint16_t  quality;		// jpeg quality 1 to 100
	char      docaption;		// TRUE or FALSE
	char      captext[128];		// appended after datetime
	char	  dodatetime;		// TRUE or FALSE
	char	  docrc;		// TRUE encode images with CRC 

	// Camera should send me 
	char      scaledown[16];       // a 1 for each scaled down image the camera sends

	// Example of how scaleddown list works:		Example numbers	
	//
	// Values set false are dont send (or if possible dont calculate)
	//   scaledown[0] = TRUE  -  Send images
	//   scaledown[1] = TRUE  -  First size wxh		800 x 600
	//            [2] = TRUE  -  Send images size/2		400 x 300
        //            [3] = TRUE  -  ... with size/4		200 x 150
	//            [4] = TRUE  -  ... with size/8		100 x 75
	//	      [5] = TRUE  -  ... with size/16		50  x 37.5

	//  Camera code only has a 'make image half size' routine, so if you
	//  make [1] true and [5] true it would have to process [2][3][4] to make [5]
	//  scale code may choose to round down or skip lines, 50x37.5 may be 37 or 36 lines

	// more to follow  exposure etc
};



// Reported by the camera when it registers
struct __attribute__((packed)) jcamsys_camera_capabilities
{
	uint16_t   modes[JC_MAX_MODES];	       // List of supported modes (jcamsys_modes.h)
	
};





// Prototypes
void jcs_camera_mode_to_wh(struct jcamsys_camerasettings* jcs, uint16_t* width, uint16_t* height);
void jc_camera_default(struct jcamsys_camerasettings* jcs);
void jc_camera_set_mode(struct jcamsys_camerasettings* jcs, int image_type, int cam, int mode, int width, int height, int update_rate_ms, int quality, int docrc);
void jc_camera_print_struct_camerasettings(struct jcamsys_camerasettings* jcs);



