// jcamsys.h
// global settings for jcamsys

#define TRUE				1
#define FALSE				0

#define JC_MAX_CAMS			16
#define JC_DEF_IMAGE_WIDTH		640
#define JC_DEF_IMAGE_HEIGHT		480
#define JC_DEF_CAMERA_FPS		3
#define JC_DEF_PREVIEW_DIVISOR		2
#define JC_MAX_WIDTH			1920
#define JC_MAX_HEIGHT			1080



// determines the size of 
#define JC_MAX_SENSORS			32					// must be >=MAX_CAMS


#define JC_MAX_COMP_IMAGESIZE		1 * (1024 * 1000)			// Maximum size of compressed image (jpeg)
#define JC_DEFAULT_TCP_PORT		8282
#define JC_KX_TCP_PORT			JC_DEFAULT_TCP_PORT + 10
#define JC_MAX_CHILDREN			128
#define JC_MAX_IPBAR_FAILS		4


// number of images stored, at the moment 2,  0=full,1=preview
#define JC_MAX_IMAGES			2
// buffer indexes for preview and full sized images
#define IFULL				0 
#define IPREV				1


#define JC_IDLEN			33					// 32 + '\0'
#define JC_MAXUSERNAME_LEN              32
#define JC_MAX_KEYSIZE			2048
#define JC_MIN_KEYSIZE			1024
#define JC_MIN_PASSWORD_LEN		8
#define JC_MAX_PASSWORD_LEN		64

#define JC_IMAGE_TYPE_JPEG		3000					// All we support at the moment

#define JC_HAVE_DATA			100
#define JC_TIMEOUT			101
#define JC_OK				0
#define JC_ERR				-1					//  generic error
#define JC_ERR_NOT_FOUND		-2
#define JC_ERR_BAD_VALUE		-3
#define JC_ERR_BAD_PASS			-9
#define JC_ERR_BAD_AUTH			-10
#define JC_ERR_BAD_MAGIC		-11
#define JC_ERR_KEY_NOT_VALID		-12
#define JC_ERR_WRITE_FAILED		-13
#define JC_ERR_READ_FAILED		-14
#define JC_ERR_BUFFER_SHORT		-15
#define JC_ERR_TOO_SLOW			-16
#define JC_ERR_TIMEOUT			-20




//#define JC_MAX_SMM			16					// server message mask
//#define JC_SMM_FRAME_NUMBER
