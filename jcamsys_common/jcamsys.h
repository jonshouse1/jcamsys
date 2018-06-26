// jcamsys.h
// global settings for jcamsys

#define TRUE				1
#define FALSE				0
#define JC_ENABLE_KX_SERVER		TRUE
#define JC_ARCHIVE_ERROR_EXIT		FALSE			// Exit on file write error

#define JC_MAX_CAMS			14
#define JC_DEF_IMAGE_WIDTH		640
#define JC_DEF_IMAGE_HEIGHT		480
#define JC_DEF_CAMERA_FPS		3
#define JC_DEF_PREVIEW_DIVISOR		2
#define JC_MAX_WIDTH			1920
#define JC_MAX_HEIGHT			1080


// camera settings
#define JC_CAMNAME_LEN			32
#define JC_CAPTEXT_LEN			64
#define JC_DTS_LEN			32



#define JC_ARCHIVE_RATE_MS		500					// 2 frames a second

// determines the size of 
#define JC_MAX_SENSORS			32					// must be >=MAX_CAMS
#define JC_MAX_REGISTRATIONS		128

#define JC_MAX_COMP_IMAGESIZE		1 * (1024 * 1000)			// Maximum size of compressed image (jpeg)
#define JC_DEFAULT_TCP_PORT		8282
#define JC_DEFAULT_HTTP_TCP_PORT	JC_DEFAULT_TCP_PORT + 1
#define JC_DEFAULT_KX_TCP_PORT		JC_DEFAULT_TCP_PORT + 10



#define JC_MAX_CHILDREN			128
#define JC_MAX_IPBAR_FAILS		4


// number of sizes, used as array index, [0]=full size, [1]=half size [2] 1/4 etc ...
#define JC_MAX_IMAGES			4

#define IFULL				0 					// img[0] is always the largest image
//#define IPREV				1

#define JC_COMMENT_LEN			2048
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
