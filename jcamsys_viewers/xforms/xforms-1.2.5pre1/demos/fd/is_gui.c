/* Form definition file generated by fdesign on Wed Apr 11 12:29:44 2018 */

#include <stdlib.h>
#include "is_gui.h"


/***************************************
 ***************************************/

FD_is_mainform *
create_form_is_mainform( void )
{
    FL_OBJECT *obj;
    FD_is_mainform *fdui = ( FD_is_mainform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->is_mainform = fl_bgn_form( FL_NO_BOX, 950, 780 );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 950, 780, "" );

    obj = fl_add_box( FL_DOWN_BOX, 5, 50, 250, 690, "" );

    fdui->canvas = obj = fl_add_canvas( FL_NORMAL_CANVAS, 260, 55, 655, 660, "" );

    fdui->vscroll = obj = fl_add_scrollbar( FL_VERT_THIN_SCROLLBAR, 920, 55, 19, 665, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, vscroll_callback, 0 );

    fdui->hscroll = obj = fl_add_scrollbar( FL_HOR_THIN_SCROLLBAR, 258, 720, 659, 20, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );
    fl_set_object_resize( obj, FL_RESIZE_ALL );
    fl_set_object_callback( obj, hscroll_callback, 0 );

    fdui->tabfolder = obj = fl_add_tabfolder( FL_TOP_TABFOLDER, 8, 53, 244, 684, "" );

    obj = fl_add_frame( FL_ENGRAVED_FRAME, 690, 745, 255, 30, "" );

    obj = fl_add_frame( FL_ENGRAVED_FRAME, 259, 3, 675, 45, "" );

    obj = fl_add_menu( FL_PULLDOWN_MENU, 10, 10, 60, 25, "File" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_lstyle( obj, FL_BOLD_STYLE );
    fl_set_object_callback( obj, filemenu_callback, 0 );
    fl_addto_menu( obj, "New" );
    fl_set_menu_item_shortcut( obj, 1, "#N" );
    fl_addto_menu( obj, "Open" );
    fl_set_menu_item_shortcut( obj, 2, "#O" );
    fl_addto_menu( obj, "Save" );
    fl_set_menu_item_shortcut( obj, 3, "#S" );
    fl_addto_menu( obj, "Save As%l" );
    fl_set_menu_item_shortcut( obj, 4, "#A" );
    fl_addto_menu( obj, "Exit" );
    fl_set_menu_item_shortcut( obj, 5, "#E" );

    fdui->progressbar = obj = fl_add_slider( FL_HOR_FILL_SLIDER, 285, 747, 156, 26, "" );
    fl_set_object_boxtype( obj, FL_FRAME_BOX );
    fl_set_object_color( obj, FL_COL1, FL_DODGERBLUE );
    fl_set_object_return( obj, FL_RETURN_END_CHANGED );
    fl_set_slider_value( obj, 0 );
    fl_set_slider_size( obj, 0.15 );

    fdui->status = obj = fl_add_text( FL_NORMAL_TEXT, 6, 745, 278, 29, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );

    fdui->errormsg = obj = fl_add_text( FL_NORMAL_TEXT, 442, 745, 245, 30, "" );
    fl_set_object_boxtype( obj, FL_DOWN_BOX );

    fl_end_form( );

    fdui->is_mainform->fdui = fdui;

    return fdui;
}

/***************************************
 ***************************************/

FD_viewform *
create_form_viewform( void )
{
    FL_OBJECT *obj;
    FD_viewform *fdui = ( FD_viewform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->viewform = fl_bgn_form( FL_NO_BOX, 240, 670 );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 240, 670, "" );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 10, 298, 224, 68, "Window Levelling" );

    fdui->doublebuffer = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 30, 80, 40, 30, "DoubleBuffer" );
    fl_set_object_callback( obj, doublebuffer_callback, 0 );

    fdui->headerinfo = obj = fl_add_browser( FL_NORMAL_BROWSER, 10, 460, 220, 120, "HeaderInfo" );
    fl_set_object_color( obj, FL_COL1, FL_YELLOW );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT_TOP );

    fdui->window = obj = fl_add_valslider( FL_HOR_NICE_SLIDER, 20, 340, 195, 18, "W" );
    fl_set_object_boxtype( obj, FL_FLAT_BOX );
    fl_set_object_color( obj, FL_COL1, FL_BLUE );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT );
    fl_set_object_callback( obj, windowlevel_callback, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 0, 4095 );
    fl_set_slider_value( obj, 256 );
    fl_set_slider_increment( obj, 1, 1 );

    fdui->level = obj = fl_add_valslider( FL_HOR_NICE_SLIDER, 20, 316, 195, 18, "L" );
    fl_set_object_boxtype( obj, FL_FLAT_BOX );
    fl_set_object_color( obj, FL_COL1, FL_BLUE );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT );
    fl_set_object_callback( obj, windowlevel_callback, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 0, 4095 );
    fl_set_slider_value( obj, 2514 );
    fl_set_slider_increment( obj, 1, 1 );

    fdui->pixmap = obj = fl_add_pixmap( FL_NORMAL_PIXMAP, 160, 585, 70, 70, "" );
    fl_set_object_boxtype( obj, FL_FRAME_BOX );

    fl_end_form( );

    fdui->viewform->fdui = fdui;

    return fdui;
}

/***************************************
 ***************************************/

FD_processform *
create_form_processform( void )
{
    FL_OBJECT *obj;
    FD_processform *fdui = ( FD_processform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->processform = fl_bgn_form( FL_NO_BOX, 240, 670 );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 240, 670, "" );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 10, 275, 220, 90, "CurrentFillColor" );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 6, 60, 229, 150, "Scale" );

    fdui->width = obj = fl_add_counter( FL_NORMAL_COUNTER, 60, 120, 120, 20, "Width" );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT );
    fl_set_counter_bounds( obj, 0.1, 100.0 );
    fl_set_counter_value( obj, 1.0 );

    fdui->height = obj = fl_add_counter( FL_NORMAL_COUNTER, 60, 140, 120, 20, "Height" );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT );
    fl_set_counter_bounds( obj, 0.1, 101.0 );
    fl_set_counter_value( obj, 1.0 );

    fdui->subpixel = obj = fl_add_choice( FL_NORMAL_CHOICE2, 12, 80, 82, 22, "" );
    fl_addto_choice( obj, "subpixel" );
    fl_addto_choice( obj, "no subpixel" );

    fdui->aspect = obj = fl_add_choice( FL_NORMAL_CHOICE2, 96, 80, 67, 22, "" );
    fl_addto_choice( obj, "aspect" );
    fl_addto_choice( obj, "exact" );
    fl_set_choice( obj, 2 );

    obj = fl_add_button( FL_NORMAL_BUTTON, 150, 175, 70, 25, "OK" );
    fl_set_object_callback( obj, scaling_callback, 0 );

    fdui->pixel = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 40, 175, 80, 25, "Pixel" );
    fl_set_object_callback( obj, switchtopixel_callback, 0 );

    fdui->red = obj = fl_add_valslider( FL_HOR_FILL_SLIDER, 30, 290, 160, 20, "" );
    fl_set_object_color( obj, FL_COL1, FL_RED );
    fl_set_object_callback( obj, colorchange_callback, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 0, 255 );
    fl_set_slider_value( obj, 205 );
    fl_set_slider_size( obj, 0.15 );

    fdui->green = obj = fl_add_valslider( FL_HOR_FILL_SLIDER, 30, 310, 160, 20, "" );
    fl_set_object_color( obj, FL_COL1, FL_GREEN );
    fl_set_object_callback( obj, colorchange_callback, 1 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 0, 255 );
    fl_set_slider_value( obj, 87 );
    fl_set_slider_size( obj, 0.15 );
    fl_set_slider_step( obj, 1 );

    fdui->blue = obj = fl_add_valslider( FL_HOR_FILL_SLIDER, 30, 330, 160, 20, "" );
    fl_set_object_color( obj, FL_COL1, FL_BLUE );
    fl_set_object_callback( obj, colorchange_callback, 2 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 0, 255 );
    fl_set_slider_value( obj, 128 );
    fl_set_slider_size( obj, 0.15 );

    fdui->colorobj = obj = fl_add_box( FL_FLAT_BOX, 193, 290, 30, 60, "" );
    fl_set_object_color( obj, FL_FREE_COL1, FL_COL1 );

    fdui->align = obj = fl_add_choice( FL_NORMAL_CHOICE2, 164, 80, 68, 22, "" );
    fl_addto_choice( obj, "center" );
    fl_addto_choice( obj, "topleft" );

    obj = fl_add_button( FL_NORMAL_BUTTON, 40, 410, 70, 25, "Sharpen" );
    fl_set_object_callback( obj, convolve_callback, (long)FLIMAGE_SHARPEN );

    obj = fl_add_button( FL_NORMAL_BUTTON, 120, 410, 70, 25, "Smooth" );
    fl_set_object_callback( obj, convolve_callback, (long)FLIMAGE_SMOOTH );

    obj = fl_add_button( FL_NORMAL_BUTTON, 40, 450, 70, 25, "Auto Crop" );
    fl_set_object_callback( obj, autocrop_callback, 0 );

    fl_end_form( );

    fdui->processform->fdui = fdui;

    return fdui;
}

/***************************************
 ***************************************/

FD_saveAs_form *
create_form_saveAs_form( void )
{
    FL_OBJECT *obj;
    FD_saveAs_form *fdui = ( FD_saveAs_form * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->saveAs_form = fl_bgn_form( FL_NO_BOX, 320, 400 );

    obj = fl_add_box( FL_UP_BOX, 0, 0, 320, 400, "" );

    fdui->formcontainer = obj = fl_add_formbrowser( FL_NORMAL_FORMBROWSER, 7, 52, 310, 340, "" );
    fl_set_object_boxtype( obj, FL_FLAT_BOX );

    fdui->outformat = obj = fl_add_choice( FL_NORMAL_CHOICE2, 65, 10, 100, 30, "Format" );
    fl_set_object_callback( obj, outformat_callback, 0 );

    fl_end_form( );

    fdui->saveAs_form->fdui = fdui;

    return fdui;
}

/***************************************
 ***************************************/

FD_annotationform *
create_form_annotationform( void )
{
    FL_OBJECT *obj;
    FD_annotationform *fdui = ( FD_annotationform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->annotationform = fl_bgn_form( FL_NO_BOX, 240, 670 );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 240, 670, "" );

    fl_end_form( );

    fdui->annotationform->fdui = fdui;

    return fdui;
}

/***************************************
 ***************************************/

FD_paintform *
create_form_paintform( void )
{
    FL_OBJECT *obj;
    FD_paintform *fdui = ( FD_paintform * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->paintform = fl_bgn_form( FL_NO_BOX, 240, 670 );

    obj = fl_add_box( FL_UP_BOX, 0, 0, 240, 670, "" );

    fl_end_form( );

    fdui->paintform->fdui = fdui;

    return fdui;
}
