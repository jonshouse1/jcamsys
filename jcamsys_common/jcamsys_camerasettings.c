// jcamsys_camerasetting.c
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "jcamsys.h"
#include "jcamsys_modes.h"
#include "jcamsys_common.h"
#include "jcamsys_cipher.h"
#include "jcamsys_statistics.h"
#include "jcamsys_sensors.h"
#include "jcamsys_camerasettings.h"
#include "jcamsys_server_settings.h"
#include "jcamsys_protocol.h"
#include "ipbar.h"
#include "jcamsys_network.h"
#include "jcamsys_images.h"
#include "jcamsys_sharedmem.h"
#include "jcamsys_sensors_prot.h"




// Some defaults
void jc_camera_default(struct jcamsys_camerasettings* jcs, int cam)
{
	int i=0;

	bzero(jcs,sizeof(struct jcamsys_camerasettings));
	jcs->yuv		= FALSE;
	jcs->update_rate_ms	= 1000;
	jcs->requested_width	= 640;
	jcs->requested_height	= 480;
	jcs->width	= 0;	
	jcs->height	= 0;
	jcs->id[0]	= 0;
	jcs->image_type = JC_IMAGE_TYPE_JPEG;
	jcs->cam	= cam;
	jcs->fliph	= FALSE;
	jcs->flipv	= FALSE;
	jcs->quality	= 75;
	jcs->docaption	= TRUE;
	sprintf(jcs->captext,"CAM %d",cam);		// default caption
	jcs->dodatetime = TRUE;
	sprintf(jcs->customdate,"%%Y-%%m-%%d %%H:%%M:%%S");

	jcs->autoexposure 	= TRUE;			// use V4l default (IE dont set)
	jcs->exposure   	= 0;
	jcs->autobrightness	= TRUE;
	jcs->brightness 	= 0;
	jcs->autocontrast	= TRUE;
	jcs->contrast		= 0;

	sprintf(jcs->captext,"CAM %d",cam);
	jcs->docrc	= FALSE;
	jcs->ircut	= 1;
	jcs->publicview = FALSE;

	for (i=0;i<JC_MAX_IMAGES;i++)
		jcs->scaledown[i] = FALSE;		// start with all off

	jcs->scaledown[0]	= TRUE;			// 640x480 - also "camera enabled"
	jcs->scaledown[1]	= TRUE;			// 320 x 240
	jcs->scaledown[2]	= TRUE;			// 160 x 120
	jcs->scaledown[3]	= TRUE;			// 80 x 60
}




void print_struct_camerasettings(struct jcamsys_camerasettings* jcs)
{
	int i=0;
	int fps=0;
	int w=0;
	int h=0;

	if (jcs->update_rate_ms>0)
		fps=1000/jcs->update_rate_ms;
        printf("cam\t\t= %d\n",		jcs->cam);
	printf("yuv\t\t= %d ",		jcs->yuv);
	if (jcs->yuv==0)
		printf("(RGB)\n");
	else	printf("\n");
        printf("docrc\t\t= %d\n",	jcs->docrc);
        printf("quality\t\t= %d\n",	jcs->quality);
        printf("id\t\t= %s\n",		jcs->id);
        printf("camname\t\t= %s\n",	jcs->camname);
	printf("image_type\t= %u\n",	jcs->image_type);
        printf("requested_width\t= %d\n",	jcs->requested_width);
        printf("requested_height= %d\n",	jcs->requested_height);
        printf("width\t\t= %d\n",	jcs->width);
        printf("height\t\t= %d\n",	jcs->height);
	printf("fliph\t\t= %d\n",	jcs->fliph);
        printf("flipv\t\t= %d\n",	jcs->flipv);

        printf("autoexposure\t= %d\n",	jcs->autoexposure);
        printf("exposure\t= %d\n",	jcs->exposure);
        printf("autobrightness\t= %d\n",jcs->autobrightness);
        printf("brightness\t= %d\n",	jcs->brightness);
        printf("autocontrast\t= %d\n",	jcs->autocontrast);
        printf("contrast\t= %d\n",	jcs->contrast);

	printf("update_rate_ms\t= %d (%d fps)\n",jcs->update_rate_ms,fps);
        printf("docaption\t= %d\n",	jcs->docaption);
        printf("captext\t\t= %s\n",	jcs->captext);
        printf("dodatetime\t= %d\n",	jcs->dodatetime);
        printf("docustomdate\t= %d\n",	jcs->docustomdate);
        printf("customdate\t= %s\n",	jcs->customdate);
        printf("publicview\t= %d ",	jcs->publicview);
	if (jcs->publicview==TRUE)
		printf("Safe for public to see\n");
	else	printf("Private\n");
	printf("dts\t\t= %s\n",		jcs->dts);

	if (jcs->scaledown[0]==TRUE)
        	printf("scaledown[0]==TRUE (enable camera)\n");

        printf("scaledown[%d]\t= [",JC_MAX_IMAGES);
	for (i=1;i<JC_MAX_IMAGES;i++)
	{
        	if (jcs->scaledown[i]==TRUE)
			printf("1");
		else	printf("0");
	}
	printf("]\n");

	for (i=0;i<JC_MAX_IMAGES;i++)
	{
		jc_img_wh(i, jcs->width, jcs->height, &w, &h);
		if (jcs->scaledown[i]==TRUE)
			printf("  img[%d]\t=  %dx%d\n",i,w,h);
	}
	fflush(stdout);
	printf("ircut\t\t= %d ",	jcs->ircut);
	if (jcs->ircut==0)
		printf("Nighttime\n");
	else	printf("Daytime\n");

	printf("\n");  fflush(stdout);
}



