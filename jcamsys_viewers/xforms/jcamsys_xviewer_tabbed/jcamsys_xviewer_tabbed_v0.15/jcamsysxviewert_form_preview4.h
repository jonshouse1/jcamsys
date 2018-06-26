/* Header file generated by fdesign on Thu Apr 12 12:20:45 2018 */

#ifndef FD_preview4_h_
#define FD_preview4_h_

#include <forms.h>

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void timer_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * preview4;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * background;
    FL_OBJECT * tim;
    FL_OBJECT * previewbm[4];
} FD_preview4;

FD_preview4 * create_form_preview4( void );

#if defined __cplusplus
}
#endif

#endif /* FD_preview4_h_ */