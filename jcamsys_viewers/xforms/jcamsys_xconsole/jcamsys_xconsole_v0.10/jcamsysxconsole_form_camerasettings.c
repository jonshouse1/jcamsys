/* Form definition file generated by fdesign on Sat Apr 21 01:08:14 2018 */

#include <stdlib.h>
#include "jcamsysxconsole_form_camerasettings.h"


/***************************************
 ***************************************/

FD_camerasettings *
create_form_camerasettings( void )
{
    FL_OBJECT *obj;
    FD_camerasettings *fdui = ( FD_camerasettings * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->camerasettings = fl_bgn_form( FL_NO_BOX, 1024, 716 );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 1024, 716, "" );

    obj = fl_add_box( FL_UP_BOX, 40, 20, 390, 680, "" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_COL1 );

    obj = fl_add_box( FL_UP_BOX, 50, 490, 370, 100, "" );
    fl_set_object_color( obj, FL_LEFT_BCOL, FL_COL1 );

    fdui->update_rate_ms = obj = fl_add_valslider( FL_HOR_BROWSER_SLIDER, 220, 40, 190, 30, "" );
    fl_set_object_callback( obj, fps_cb, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 1, 30 );
    fl_set_slider_value( obj, 2 );
    fl_set_slider_step( obj, 1 );
    fl_set_slider_increment( obj, 1, 1 );

    fdui->update_rate_mstext = obj = fl_add_text( FL_NORMAL_TEXT, 60, 40, 120, 30, "Update rate (ms)" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->resolutiontext = obj = fl_add_text( FL_NORMAL_TEXT, 60, 340, 100, 30, "Resolution" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->iwidth = obj = fl_add_input( FL_INT_INPUT, 100, 390, 80, 30, "width" );

    fdui->iheight = obj = fl_add_input( FL_INT_INPUT, 230, 390, 80, 30, "height" );

    fdui->id = obj = fl_add_text( FL_NORMAL_TEXT, 70, 650, 340, 40, "ID" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->fliph = obj = fl_add_button( FL_PUSH_BUTTON, 380, 440, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, fliph_cb, 0 );

    obj = fl_add_text( FL_NORMAL_TEXT, 320, 440, 50, 30, "fliph" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    obj = fl_add_text( FL_NORMAL_TEXT, 230, 440, 40, 30, "flipv" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->flipv = obj = fl_add_button( FL_PUSH_BUTTON, 280, 440, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, flipv_cb, 0 );

    fdui->docaption = obj = fl_add_button( FL_PUSH_BUTTON, 130, 610, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, docaption_cb, 0 );

    fdui->captext = obj = fl_add_input( FL_NORMAL_INPUT, 190, 610, 220, 30, "" );

    fdui->ircuttest = obj = fl_add_button( FL_PUSH_BUTTON, 380, 290, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, ircuttest_cb, 0 );

    obj = fl_add_text( FL_NORMAL_TEXT, 270, 290, 100, 30, "IRCUT TEST" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    obj = fl_add_text( FL_NORMAL_TEXT, 170, 40, 50, 30, "FPS" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );

    fdui->largeviewcanvas = obj = fl_add_canvas( FL_NORMAL_CANVAS, 460, 70, 530, 380, "" );

    obj = fl_add_text( FL_NORMAL_TEXT, 150, 290, 90, 30, "Light/Dark" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 90, 120, 30, "Quality" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->quality = obj = fl_add_valslider( FL_HOR_BROWSER_SLIDER, 220, 90, 190, 30, "" );
    fl_set_object_callback( obj, qual_cb, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 10, 100 );
    fl_set_slider_value( obj, 75 );
    fl_set_slider_step( obj, 1 );
    fl_set_slider_increment( obj, 1, 1 );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 140, 80, 30, "Exposure" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->exposure = obj = fl_add_valslider( FL_HOR_BROWSER_SLIDER, 220, 140, 190, 30, "" );
    fl_set_object_callback( obj, exp_cb, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 10, 100 );
    fl_set_slider_value( obj, 75 );
    fl_set_slider_step( obj, 1 );
    fl_set_slider_increment( obj, 1, 1 );

    fdui->autoexposure = obj = fl_add_button( FL_PUSH_BUTTON, 150, 140, 40, 30, "Auto" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, cs_cb, 1 );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 190, 80, 30, "Brightness" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->autobrightness = obj = fl_add_button( FL_PUSH_BUTTON, 150, 190, 40, 30, "Auto" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, cs_cb, 2 );

    fdui->brightness = obj = fl_add_valslider( FL_HOR_BROWSER_SLIDER, 220, 190, 190, 30, "" );
    fl_set_object_callback( obj, br_cb, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 10, 100 );
    fl_set_slider_value( obj, 75 );
    fl_set_slider_step( obj, 1 );
    fl_set_slider_increment( obj, 1, 1 );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 240, 80, 30, "Contrast" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    fdui->autocontrast = obj = fl_add_button( FL_PUSH_BUTTON, 150, 240, 40, 30, "Auto" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, cs_cb, 3 );

    fdui->contrast = obj = fl_add_valslider( FL_HOR_BROWSER_SLIDER, 220, 240, 190, 30, "" );
    fl_set_object_callback( obj, co_cb, 0 );
    fl_set_object_return( obj, FL_RETURN_CHANGED );
    fl_set_slider_precision( obj, 0 );
    fl_set_slider_bounds( obj, 10, 100 );
    fl_set_slider_value( obj, 75 );
    fl_set_slider_step( obj, 1 );
    fl_set_slider_increment( obj, 1, 1 );

    fdui->customdate = obj = fl_add_input( FL_NORMAL_INPUT, 190, 550, 220, 30, "" );

    fdui->docustomdate = obj = fl_add_button( FL_PUSH_BUTTON, 130, 550, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, customdate_cb, 0 );

    fdui->dodatetime = obj = fl_add_button( FL_PUSH_BUTTON, 130, 500, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, dodatetime_cb, 0 );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 500, 40, 30, "Date" );
    fl_set_object_color( obj, FL_LEFT_BCOL, FL_MCOL );

    fdui->apply = obj = fl_add_button( FL_PUSH_BUTTON, 340, 390, 70, 30, "Apply" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, apply_cb, 0 );

    fdui->cameralabel = obj = fl_add_text( FL_NORMAL_TEXT, 530, 10, 100, 40, "CAM 01" );
    fl_set_object_lcolor( obj, FL_CHARTREUSE );
    fl_set_object_lsize( obj, FL_LARGE_SIZE );
    fl_set_object_lstyle( obj, FL_BOLD_STYLE );

    fdui->ipaddr = obj = fl_add_text( FL_NORMAL_TEXT, 850, 10, 140, 30, "XXX.XXX.XXX.XXX" );
    fl_set_object_color( obj, FL_DARKER_COL1, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    obj = fl_add_text( FL_NORMAL_TEXT, 790, 10, 50, 30, "Server" );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    obj = fl_add_canvas( FL_NORMAL_CANVAS, 460, 520, 160, 130, "" );

    fdui->largeviewtext = obj = fl_add_text( FL_NORMAL_TEXT, 460, 460, 410, 30, "640 x 480" );

    obj = fl_add_text( FL_NORMAL_TEXT, 460, 660, 80, 30, "320 x 200" );

    obj = fl_add_text( FL_NORMAL_TEXT, 650, 660, 80, 30, "160 x 100" );

    obj = fl_add_text( FL_NORMAL_TEXT, 840, 660, 80, 30, "80 x 50" );

    fdui->leftbut = obj = fl_add_pixmapbutton( FL_NORMAL_BUTTON, 460, 10, 60, 40, "" );
    fl_set_object_boxtype( obj, FL_NO_BOX );
    fl_set_object_callback( obj, changecam_cb, 1 );
    fl_set_pixmapbutton_file( obj, "aleft.xpm" );

    fdui->rightbut = obj = fl_add_pixmapbutton( FL_NORMAL_BUTTON, 640, 10, 60, 40, "" );
    fl_set_object_boxtype( obj, FL_NO_BOX );
    fl_set_object_callback( obj, changecam_cb, 2 );
    fl_set_pixmapbutton_file( obj, "aright.xpm" );

    obj = fl_add_canvas( FL_NORMAL_CANVAS, 650, 520, 160, 130, "" );

    obj = fl_add_canvas( FL_NORMAL_CANVAS, 840, 520, 160, 130, "" );

    obj = fl_add_text( FL_NORMAL_TEXT, 830, 460, 120, 30, "Clip / Scale" );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    fdui->clipscale = obj = fl_add_button( FL_PUSH_BUTTON, 960, 460, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, clipscale_cb, 0 );

    fdui->publicview = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 70, 296, 30, 20, "Public" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, publicview_cb, 0 );

    fdui->quality_default = obj = fl_add_button( FL_PUSH_BUTTON, 150, 90, 40, 30, "Def" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, quality_default_cb, 1 );

    obj = fl_add_text( FL_NORMAL_TEXT, 160, 340, 210, 30, "Same resolution on all cameras" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    fdui->samerescams = obj = fl_add_button( FL_PUSH_BUTTON, 380, 340, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, sameres_cb, 0 );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 550, 60, 30, "Custom" );
    fl_set_object_color( obj, FL_LEFT_BCOL, FL_MCOL );

    obj = fl_add_text( FL_NORMAL_TEXT, 60, 610, 60, 30, "Caption" );
    fl_set_object_color( obj, FL_TOP_BCOL, FL_MCOL );

    obj = fl_add_text( FL_NORMAL_TEXT, 190, 500, 180, 30, "Same date on all cameras" );
    fl_set_object_color( obj, FL_LEFT_BCOL, FL_MCOL );
    fl_set_object_lalign( obj, FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

    fdui->samedatecams = obj = fl_add_button( FL_PUSH_BUTTON, 380, 500, 30, 30, "" );
    fl_set_object_color( obj, FL_COL1, FL_CHARTREUSE );
    fl_set_object_callback( obj, samedate_cb, 0 );

    fl_end_form( );

    fdui->camerasettings->fdui = fdui;

    return fdui;
}
