// jcamsys_camerasetting.c
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

//#include "jcamsys.h"
//#include "jcamsys_modes.h"
//#include "jcamsys_common.h"
//#include "jcamsys_cipher.h"
//#include "jcamsys_network.h"
//#include "jcamsys_sensors.h"
//#include "jcamsys_modes.h"

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "ipbar.h"
#include "jcamsys_protocol.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_network.h"





// For a given mode return width and height
void jcs_camera_mode_to_wh(struct jcamsys_camerasettings* jcs, uint16_t* width, uint16_t* height)
{
	*width=0;
	*height=0;
	//sscanf (&jc_modes[jcs->mode][0],"%d x %d",width,height);
	sscanf (jc_modes[jcs->mode],"%d x %d",(int*)width,(int*)height);
}




// width and height are ignored unless mode=0 (JC_MODE_CUSTOM)
void jc_camera_set_mode(struct jcamsys_camerasettings* jcs, int image_type, int cam, int mode, int width, int height, int update_rate_ms, int quality, int docrc)
{
	int i=0;

	bzero(jcs,sizeof(struct jcamsys_camerasettings));

	jcs->width	= width;			// if mode is !=0 then =0
	jcs->height	= height;
	jcs->mode	= mode;

	// set width height based on 'mode'
	if (jcs->mode != JC_MODE_CUSTOM)
		jcs_camera_mode_to_wh(jcs, &jcs->width, &jcs->height);

	jcs->id[0]	= 0;
	jcs->changed	= TRUE;
	jcs->image_type = image_type;
	jcs->cam	= cam;
	jcs->fliph	= FALSE;
	jcs->flipv	= FALSE;
	jcs->update_rate_ms	= update_rate_ms;
	jcs->quality	= quality;
	jcs->docaption	= TRUE;
	jcs->dodatetime = TRUE;

	sprintf(jcs->captext,"CAM %d",jcs->cam);
	jcs->docrc	= docrc;

	jcs->scaledown[0]	= TRUE;			// enable sending of images
	jcs->scaledown[1]	= TRUE;			// send (in this case) 640 x 480
	jcs->scaledown[2]	= TRUE;			// 320 x 240;

	for (i=3;i<15;i++)
		jcs->scaledown[i] = FALSE;		// all others off
}



// Just a sensible set of defaults, note cam=0 so that the server will ignore
// anything trying to operate in this mode for real
void jc_camera_default(struct jcamsys_camerasettings* jcs)
{
	jc_camera_set_mode(jcs, JC_IMAGE_TYPE_JPEG, 0, JC_MODE_640x480, 0, 0, 1000, 75, FALSE);
}



void jc_camera_print_struct_camerasettings(struct jcamsys_camerasettings* jcs)
{
	int i=0;
	int fps=0;

	if (jcs->update_rate_ms>0)
		fps=1000/jcs->update_rate_ms;
        printf("cam\t\t= %d\n",		jcs->cam);
        printf("docrc\t\t= %d\n",	jcs->docrc);
        printf("quality\t\t= %d\n",	jcs->quality);
        printf("id\t\t= %s\n",		jcs->id);
        printf("camname\t\t= %s\n",	jcs->camname);
	printf("changed\t\t= %d\n",	jcs->changed);
	printf("image_type\t= %u\n",	jcs->image_type);
	printf("mode\t\t= %d\n",	jcs->mode);
        printf("width\t\t= %d\n",	jcs->width);
        printf("height\t\t= %d\n",	jcs->height);
	printf("fliph\t\t= %d\n",	jcs->fliph);
        printf("flipv\t\t= %d\n",	jcs->flipv);
	printf("update_rate_ms\t= %d (%d fps)\n",jcs->update_rate_ms,fps);
        printf("docaption\t= %d\n",	jcs->docaption);
        printf("captext\t\t= %s\n",	jcs->captext);
        printf("dodatetime\t= %d\n",	jcs->dodatetime);

        // Camera should send me 
        printf("scaledown[16]\t= [");
	for (i=0;i<16;i++)
        	printf("%d",jcs->scaledown[i]);
	printf("]\n");
	printf("\n");  fflush(stdout);
}




