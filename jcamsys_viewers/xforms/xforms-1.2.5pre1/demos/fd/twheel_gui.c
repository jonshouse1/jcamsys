/* Form definition file generated by fdesign on Wed Apr 11 12:30:00 2018 */

#include <stdlib.h>
#include "twheel_gui.h"


/***************************************
 ***************************************/

FD_twheelform *
create_form_twheelform( void )
{
    FL_OBJECT *obj;
    FD_twheelform *fdui = ( FD_twheelform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->twheelform = fl_bgn_form( FL_NO_BOX, 220, 260 );

    obj = fl_add_box( FL_UP_BOX, 0, 0, 220, 260, "" );

    obj = fl_add_frame( FL_ENGRAVED_FRAME, 15, 70, 190, 130, "" );

    fdui->vert = obj = fl_add_thumbwheel( FL_VERT_THUMBWHEEL, 30, 90, 20, 100, "" );
    fl_set_object_callback( obj, valchange_cb, 0 );
    fl_set_thumbwheel_step( obj, 0.01 );

    fdui->hor = obj = fl_add_thumbwheel( FL_HOR_THUMBWHEEL, 60, 130, 120, 23, "" );
    fl_set_object_callback( obj, valchange_cb, 0 );
    fl_set_thumbwheel_step( obj, 0.01 );

    fdui->report = obj = fl_add_text( FL_NORMAL_TEXT, 60, 90, 120, 30, "" );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );

    fdui->returnsetting = obj = fl_add_choice( FL_NORMAL_CHOICE2, 35, 20, 160, 30, "" );
    fl_set_object_boxtype( obj, FL_EMBOSSED_BOX );
    fl_set_object_callback( obj, returnchange_cb, 0 );
    fl_addto_choice( obj, "End & Changed" );
    fl_addto_choice( obj, "Whenever Changed" );
    fl_addto_choice( obj, "Always At End" );
    fl_addto_choice( obj, "Always" );
    fl_set_choice( obj, 2 );

    obj = fl_add_button( FL_NORMAL_BUTTON, 120, 215, 80, 30, "Enough" );

    fl_end_form( );

    fdui->twheelform->fdui = fdui;

    return fdui;
}
