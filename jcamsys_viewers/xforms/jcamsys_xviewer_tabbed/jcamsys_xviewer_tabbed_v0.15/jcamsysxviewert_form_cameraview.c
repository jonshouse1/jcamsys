/* Form definition file generated by fdesign on Wed Apr 11 14:12:21 2018 */

#include <stdlib.h>
#include "jcamsys.h"
#include "jcamsysxviewert.h"
#include "jcamsysxviewert_form_cameraview.h"


/***************************************
 ***************************************/

FD_cameraview *
create_form_cameraview( void )
{
    FL_OBJECT *obj;
    FD_cameraview *fdui = ( FD_cameraview * ) fl_malloc( sizeof *fdui );
    char st[8];
    int i;
    int x;

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    //fdui->cameraview = fl_bgn_form( FL_NO_BOX, 645, 523 );
    fdui->cameraview = fl_bgn_form( FL_NO_BOX, JC_MAX_WIDTH+JC_PLUSX, JC_MAX_HEIGHT+JC_PLUSY );

    //obj = fl_add_box( FL_FLAT_BOX, -1, 0, 645, 523, "" );
    obj = fl_add_box( FL_FLAT_BOX, -1, 0, JC_MAX_WIDTH+JC_PLUSX, JC_MAX_HEIGHT+JC_PLUSY, "" );
    fl_set_object_color( obj, FL_DODGERBLUE, FL_BLACK );				// acts as tab highlight

    //obj = fl_add_box( FL_UP_BOX, 0, 477, 645, 46, "" );
    fdui->background = obj = fl_add_box( FL_UP_BOX, 0, 0, JC_MAX_WIDTH+JC_PLUSX, JC_MAX_HEIGHT+JC_PLUSY, "" );
    fl_set_object_color( obj, FL_BLACK, FL_BLACK );

    fdui->largeviewcanvas = obj = fl_add_bitmap( FL_NORMAL_BITMAP, 0, 0, JC_MAX_WIDTH, JC_MAX_HEIGHT, "" );

    
    for (i=1;i<JC_MAX_CAMS+1;i++)
    {
    	//fdui->bcam1[i] = obj = fl_add_button( FL_NORMAL_BUTTON, 108, 486, 30, 30, "1" );
        sprintf(st,"%d",i);
        //x=(640/JC_MAX_CAMS+2)*(i-1)+5;
	x=((640-40)/JC_MAX_CAMS)*(i-1)+5;
    	fdui->bcam[i] = obj = fl_add_button( FL_NORMAL_BUTTON, x, 486, 30, 30, st );
    	fl_set_object_boxtype( obj, FL_FRAME_BOX );
    	fl_set_object_color( obj, FL_BLACK, FL_ALICEBLUE );
    	fl_set_object_lcolor( obj, FL_BOTTOM_BCOL );
    	fl_set_object_lsize( obj, FL_LARGE_SIZE );
    	fl_set_object_callback( obj, camchange, i );
	if (x>590)			// Create all the buttons but hide some so the + button will fit
		fl_hide_object(obj);
    }

    fdui->newviewer = obj = fl_add_button( FL_NORMAL_BUTTON, 640-35, 486, 30, 30, "+" );
    fl_set_object_boxtype( obj, FL_FRAME_BOX );
    fl_set_object_color( obj, FL_BLACK, FL_ALICEBLUE );
    fl_set_object_lcolor( obj, FL_BOTTOM_BCOL );
    fl_set_object_lsize( obj, FL_LARGE_SIZE );
    fl_set_object_callback( obj, newme, 10 );


    fdui->tim = obj = fl_add_timer( FL_HIDDEN_TIMER, 0, 0, 27, 27, "timer" );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );
    fl_set_object_callback( obj, timer_cb, 0 );

    fdui->textbox = obj = fl_add_text( FL_NORMAL_TEXT, 36, 18, 576, 72, "" );
    fl_set_object_color( obj, FL_BLACK, FL_TOMATO );
    fl_set_object_lcolor( obj, FL_ALICEBLUE );
    fl_set_object_lsize( obj, FL_MEDIUM_SIZE );


    fl_end_form( );
    fdui->cameraview->fdui = fdui;
    return fdui;
}
