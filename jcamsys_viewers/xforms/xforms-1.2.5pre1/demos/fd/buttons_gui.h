/* Header file generated by fdesign on Wed Apr 11 12:29:26 2018 */

#ifndef FD_buttform_h_
#define FD_buttform_h_

#include  "include/forms.h" 

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void done_cb( FL_OBJECT *, long );
void bw_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * buttform;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * backface;
    FL_OBJECT * done;
    FL_OBJECT * objsgroup;
    FL_OBJECT * bbutt;
    FL_OBJECT * pbutt;
    FL_OBJECT * bw_obj;
} FD_buttform;

FD_buttform * create_form_buttform( void );

#if defined __cplusplus
}
#endif

#endif /* FD_buttform_h_ */
