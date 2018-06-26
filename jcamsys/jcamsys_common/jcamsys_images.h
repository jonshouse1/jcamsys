// jcamsys_images.h

#include <jpeglib.h>
#include <setjmp.h>
#include <jerror.h>




// Prototypes
void decodejpeg_error_exit (j_common_ptr cinfo);
int write_ppm_file(char *filename, unsigned char *bmp_buffer, int width, int height);
int decode_jpeg(unsigned char *jpg_buffer, unsigned long jpg_size, int *w, int *h, int *oc,unsigned char *bmp_buffer);
int write_JPEG_file(char * filename, JSAMPLE * image_buffer, int image_width, int image_height, int quality);
int encode_jpeg_tomem(JSAMPLE *image_buffer, unsigned char *destbuf, int image_width, int image_height, int quality);
void deinterlace(int sml,char *data,int width,int height);
void halfwidth(char *data,int width,int height,int average);
void scaleimage(int sml,char *data, int *w, int *h, int sc);
void add_text(char *GRAB_TEXT,char *image, int width, int height, int linesdown, int colsaccross, int R,int G,int B,int rev);
void swap_rgb24(char *mem, int n);
unsigned short int rgbto565(unsigned int R, unsigned int G, unsigned int B);
void plotpixel24(char *framebuffer, int fbwidth, int fbheight, int X, int Y, unsigned char R, unsigned char G, unsigned char B);
void jc_RGBsolidcolour(unsigned char *data, int datalen,  unsigned char R,unsigned char G,unsigned char B);
void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst);


