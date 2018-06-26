/* Form definition file generated by fdesign on Wed Apr 11 12:29:55 2018 */

#include <stdlib.h>
#include "scrollbar_gui.h"


/***************************************
 ***************************************/

FD_scb *
create_form_scb( void )
{
    FL_OBJECT *obj;
    FD_scb *fdui = ( FD_scb * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->scb = fl_bgn_form( FL_NO_BOX, 470, 230 );

    obj = fl_add_box( FL_UP_BOX, 0, 0, 470, 230, "" );

    fdui->hor = obj = fl_add_scrollbar( FL_HOR_SCROLLBAR, 30, 15, 230, 17, "HOR_SCROLLBAR" );
    fl_set_object_lsize( obj, FL_TINY_SIZE );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );

    fdui->hor_thin = obj = fl_add_scrollbar( FL_HOR_THIN_SCROLLBAR, 30, 60, 230, 18, "HOR_THIN_SCROLLBAR" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_lsize( obj, FL_TINY_SIZE );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );
    fl_set_scrollbar_value( obj, 0.11 );

    fdui->hor_nice = obj = fl_add_scrollbar( FL_HOR_NICE_SCROLLBAR, 30, 110, 230, 18, "HOR_NICE_SCROLLBAR" );
    fl_set_object_boxtype( obj, FL_FRAME_BOX );
    fl_set_object_lsize( obj, FL_TINY_SIZE );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );

    fdui->vert = obj = fl_add_scrollbar( FL_VERT_SCROLLBAR, 300, 10, 17, 185, "" );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );

    fdui->vert_thin = obj = fl_add_scrollbar( FL_VERT_THIN_SCROLLBAR, 338, 10, 17, 185, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );

    fdui->hide = obj = fl_add_button( FL_NORMAL_BUTTON, 20, 195, 80, 25, "Hide" );
    fl_set_object_callback( obj, hide_cb, 0 );

    fdui->deactivate = obj = fl_add_button( FL_NORMAL_BUTTON, 100, 195, 80, 25, "Deactivate" );
    fl_set_object_callback( obj, deactivate_cb, 0 );

    obj = fl_add_button( FL_NORMAL_BUTTON, 200, 195, 80, 25, "Done" );
    fl_set_object_callback( obj, done_cb, 0 );

    fdui->vert_nice = obj = fl_add_scrollbar( FL_VERT_NICE_SCROLLBAR, 370, 10, 17, 185, "" );
    fl_set_object_boxtype( obj, FL_FRAME_BOX );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );
    fl_set_scrollbar_value( obj, 1 );

    obj = fl_add_scrollbar( FL_HOR_PLAIN_SCROLLBAR, 30, 155, 230, 18, "HOR_PLAIN_SCROLLBAR" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_lsize( obj, FL_TINY_SIZE );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );
    fl_set_scrollbar_value( obj, 0.77 );
    fl_set_scrollbar_size( obj, 0.20 );

    obj = fl_add_scrollbar( FL_VERT_PLAIN_SCROLLBAR, 410, 10, 17, 185, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, noop_cb, 0 );
    fl_set_scrollbar_value( obj, 0.97 );
    fl_set_scrollbar_size( obj, 0.20 );

    fl_end_form( );

    fdui->scb->fdui = fdui;

    return fdui;
}
