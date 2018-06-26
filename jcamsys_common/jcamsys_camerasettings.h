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
	char		id[JC_IDLEN];
	uint16_t	cam;			// Camera number 1..JC_MAX_CAMS
	char		camname[JC_CAMNAME_LEN];
	uint16_t	image_type;		// only JC_IMAGE_TYPE_JPEG at the moment

	uint16_t	requested_width;
	uint16_t	requested_height;
	uint16_t	width;
	uint16_t	height;

	char		docaption;		// TRUE or FALSE
	char		captext[JC_CAPTEXT_LEN];// appended after datetime
	char		dodatetime;		// TRUE or FALSE
	char		dts[JC_DTS_LEN];	// initial date time string
	char		docustomdate;
	char		customdate[JC_DTS_LEN];
	char		docrc;			// TRUE encode images with CRC 

        uint16_t	update_rate_ms;		// ms between each frame
	unsigned char	quality;		// jpeg quality 1 to 100
	char	  	yuv;			// TRUE V4l using YUV, false RGB
	char		publicview;		// TRUE = strangers would be ok viewing
	char		ircut;			// 0 = Night, 1=Day
	char	  	fliph;			// T or F, T=Camera is upside down
	char	  	flipv;			// T or F, T=Camera is mirrored left to right

	// V4l2 values
	char		autoexposure;		// TRUE exposure value that follows is used
	unsigned char	exposure;		// value 0 to 100
	char		autobrightness;	
	unsigned char	brightness;		// value 0 to 100
	char		autocontrast;
	unsigned char	contrast;		// value 0 to 100


	// Camera should send me 
	char      scaledown[JC_MAX_IMAGES];	// a 1 for each scaled down image the camera sends

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
};





// Prototypes
void jc_camera_default(struct jcamsys_camerasettings* jcs, int docrc);
void print_struct_camerasettings(struct jcamsys_camerasettings* jcs);
int jc_load_camerasettings(struct jcamsys_camerasettings* cs, char *filename, int update_rate_ms, int quality, int width, int height);


