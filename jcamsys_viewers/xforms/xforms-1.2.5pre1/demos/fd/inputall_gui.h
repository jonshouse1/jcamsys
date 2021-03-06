/* Header file generated by fdesign on Wed Apr 11 12:29:43 2018 */

#ifndef FD_input_h_
#define FD_input_h_

#include  "include/forms.h" 

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void input_cb( FL_OBJECT *, long );
void done_cb( FL_OBJECT *, long );
void hide_show_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * input;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * norminput;
    FL_OBJECT * intinput;
    FL_OBJECT * floatinput;
    FL_OBJECT * dateinput;
    FL_OBJECT * secretinput;
    FL_OBJECT * multiinput;
    FL_OBJECT * report;
} FD_input;

FD_input * create_form_input( void );

#if defined __cplusplus
}
#endif

#endif /* FD_input_h_ */
