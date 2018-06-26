/* Header file generated by fdesign on Sun Apr 22 13:56:10 2018 */

#ifndef FD_serversettings_h_
#define FD_serversettings_h_

#include <forms.h>

#if defined __cplusplus
extern "C"
{
#endif

/* Callbacks, globals and object handlers */

void enable_archiver_cb( FL_OBJECT *, long );
void exit_on_archive_error_cb( FL_OBJECT *, long );
void archive_verbose_cb( FL_OBJECT *, long );
void archive_rate_fps_cb( FL_OBJECT *, long );
void archive_path_cb( FL_OBJECT *, long );
void sarchive_rate_fps_cb( FL_OBJECT *, long );
void archive_image_size_cb( FL_OBJECT *, long );
void ssapply_cb( FL_OBJECT *, long );
void docrc_cb( FL_OBJECT *, long );
void enable_kx_cb( FL_OBJECT *, long );
void enable_http_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * serversettings;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * enable_archiver;
    FL_OBJECT * exit_on_archive_error;
    FL_OBJECT * archive_verbose;
    FL_OBJECT * archive_rate_fps;
    FL_OBJECT * archive_path;
    FL_OBJECT * sarchive_rate_fps;
    FL_OBJECT * archive_image_size;
    FL_OBJECT * archive_size;
    FL_OBJECT * resolution;
    FL_OBJECT * archive_rate_ms;
    FL_OBJECT * sarchive_rate_ms;
    FL_OBJECT * comment;
    FL_OBJECT * archiving_ok;
    FL_OBJECT * ssapply;
    FL_OBJECT * server_tcp_port;
    FL_OBJECT * http_server_tcp_port;
    FL_OBJECT * docrc;
    FL_OBJECT * enable_keyexchange;
    FL_OBJECT * enable_http;
} FD_serversettings;

FD_serversettings * create_form_serversettings( void );

#if defined __cplusplus
}
#endif

#endif /* FD_serversettings_h_ */
