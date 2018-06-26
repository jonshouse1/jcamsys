/* Form definition file generated by fdesign on Wed Apr 11 13:12:11 2018 */

#include <stdlib.h>
#include "jcamsysxviewert_form_main.h"

#include "jcamsys.h"
#include "jcamsysxviewert.h"
#include "jcamsysxviewert_form_cameraview.h"
#include "jcamsysxviewert_form_preview4.h"
#include "jcamsysxviewert_form_preview9.h"
#include "jcamsysxviewert_form_sensors.h"



/***************************************


 ***************************************/

extern FD_cameraview   *fd_cameraview;
extern FD_preview4     *fd_preview4;
extern FD_preview9     *fd_preview9;
extern FD_sensors      *fd_sensors;
extern int app_width;
extern int app_height;


FD_main *
create_form_main( void )
{
    	FL_OBJECT *obj;
    	FD_main *fdui = ( FD_main * ) fl_malloc( sizeof *fdui );

    	fdui->vdata = fdui->cdata = NULL;
    	fdui->ldata = 0;

    	fdui->main  = fl_bgn_form( FL_NO_BOX, app_width, app_height );

    	//obj = fl_add_box( FL_FLAT_BOX, 0, 0, 641, 523, "" );
    	//fl_set_object_color( obj, FL_BLACK, FL_BLACK );

        fdui->tabs = obj = fl_add_tabfolder(FL_TOP_TABFOLDER,0,0,JC_MAX_WIDTH+JC_PLUSX,JC_MAX_HEIGHT+JC_PLUSY,"");
	fl_set_object_boxtype( obj, FL_NO_BOX );
    	fl_set_object_color( obj, FL_BLACK, FL_BLACK );
    	fl_set_object_lcolor( obj, FL_WHITE );
    	fl_set_object_callback( obj, tabs_cb, 0 );

        fl_addto_tabfolder(obj,"CAMERAS",fd_cameraview->cameraview);
        fl_addto_tabfolder(obj,"PREVIEW4",fd_preview4->preview4);
        fl_addto_tabfolder(obj,"PREVIEW9",fd_preview9->preview9);
        fl_addto_tabfolder(obj,"SENSORS",fd_sensors->sensors);


    	fl_end_form( );
    	fdui->main->fdui = fdui;
    	return fdui;
}



