/* Header file generated by fdesign on Sat Apr 21 01:08:14 2018 */

#ifndef FD_camerasettings_h_
#define FD_camerasettings_h_

#include <forms.h>

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void fps_cb( FL_OBJECT *, long );
void fliph_cb( FL_OBJECT *, long );
void flipv_cb( FL_OBJECT *, long );
void docaption_cb( FL_OBJECT *, long );
void ircuttest_cb( FL_OBJECT *, long );
void qual_cb( FL_OBJECT *, long );
void exp_cb( FL_OBJECT *, long );
void cs_cb( FL_OBJECT *, long );
void br_cb( FL_OBJECT *, long );
void co_cb( FL_OBJECT *, long );
void customdate_cb( FL_OBJECT *, long );
void dodatetime_cb( FL_OBJECT *, long );
void apply_cb( FL_OBJECT *, long );
void changecam_cb( FL_OBJECT *, long );
void clipscale_cb( FL_OBJECT *, long );
void publicview_cb( FL_OBJECT *, long );
void quality_default_cb( FL_OBJECT *, long );
void sameres_cb( FL_OBJECT *, long );
void samedate_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * camerasettings;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * update_rate_ms;
    FL_OBJECT * update_rate_mstext;
    FL_OBJECT * resolutiontext;
    FL_OBJECT * iwidth;
    FL_OBJECT * iheight;
    FL_OBJECT * id;
    FL_OBJECT * fliph;
    FL_OBJECT * flipv;
    FL_OBJECT * docaption;
    FL_OBJECT * captext;
    FL_OBJECT * ircuttest;
    FL_OBJECT * largeviewcanvas;
    FL_OBJECT * quality;
    FL_OBJECT * exposure;
    FL_OBJECT * autoexposure;
    FL_OBJECT * autobrightness;
    FL_OBJECT * brightness;
    FL_OBJECT * autocontrast;
    FL_OBJECT * contrast;
    FL_OBJECT * customdate;
    FL_OBJECT * docustomdate;
    FL_OBJECT * dodatetime;
    FL_OBJECT * apply;
    FL_OBJECT * cameralabel;
    FL_OBJECT * ipaddr;
    FL_OBJECT * largeviewtext;
    FL_OBJECT * leftbut;
    FL_OBJECT * rightbut;
    FL_OBJECT * clipscale;
    FL_OBJECT * publicview;
    FL_OBJECT * quality_default;
    FL_OBJECT * samerescams;
    FL_OBJECT * samedatecams;
} FD_camerasettings;

FD_camerasettings * create_form_camerasettings( void );

#if defined __cplusplus
}
#endif

#endif /* FD_camerasettings_h_ */
