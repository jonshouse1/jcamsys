// jcamsys_images.c
// jpeg encode, decode, RGB24 image sclae etc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include <jpeglib.h>
#include <setjmp.h>
#include <jerror.h>

#include "jcamsys.h"
//#include "jcamsys_modes.h"
//#include "jcamsys_camerasettings.h"



// For an image of fw x fh what is the scaled down size for img=N
int jc_img_wh(int img, int fw, int fh, int*wo, int *ho)
{
	int i=0;
	int w=0;
	int h=0;
	*wo=0;
	*ho=0;

	if (fw>0)
		w=fw;
	else	w=640;
	if (fh>0)
		h=fh;
	else	h=480;
	for (i=0;i<JC_MAX_IMAGES;i++)
	{
		if (i==img)
		{
			*wo=w;
			*ho=h;
			return(JC_OK);
		}
		w=w/2;
		h=h/2;
	}
	return(JC_ERR);
}




struct my_error_mgr
{
        struct jpeg_error_mgr pub;
        sigjmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;



void decodejpeg_error_exit (j_common_ptr cinfo)
{
        my_error_ptr myerr = (my_error_ptr) cinfo->err;

        printf("%d: decode_jpeg: error - ",getpid()); fflush(stdout);
        (*cinfo->err->output_message) (cinfo);
        fflush(stdout);

        // Return to address marked by setjmp
        longjmp(myerr->setjmp_buffer,1);
}




// Take an RGB 24 bit bitmap buffer, write it as a PPM file (gimp will read it)
int write_ppm_file(char *filename, unsigned char *bmp_buffer, int width, int height)
{
	char buf[1024];
	int rc=0;
	int fd=-1;
	int bs= width*height*3;
	int w=0;

	fd = open(filename, O_CREAT | O_WRONLY, 0666);
	if (fd<0)
		return(-1);
	rc = sprintf(buf, "P6 %d %d 255\n", width, height);
	w=write(fd, buf, rc); 							// Write the PPM image header before data
	if (w<rc)
	{
		close(fd);
		return(JC_ERR);
	}
	w=write(fd, bmp_buffer, bs);	 					// Write out all RGB pixel data
	close(fd);
	if (w<rc)
		return(JC_ERR);
	return(bs);
}





// jpegin  (jpeg data, in memory)
// jpeg_size (size of jpeg data)
// bmp_buffer (output buffer, make sure it is large enough)
// Return value is size of output buffer or <=0 if jpeg can not be decoded
// w,h and oc,return width and height and output_components (RGB=3) to caller
int decode_jpeg(unsigned char *jpg_buffer, unsigned long jpg_size, int *w, int *h, int *oc,unsigned char *bmp_buffer)
{
	int rc;
	unsigned long bmp_size;

	// Variables for the decompressor itself
	struct jpeg_decompress_struct cinfo;
	//struct jpeg_error_mgr jerr;
	struct my_error_mgr jerr;

	// Variables for the output buffer, and how long each row is
	int row_stride;
	if (jpg_size<16)
		return(JC_ERR);

	// Check it looks like a jpeg before passing it decoder
	// DumpHex(jpg_buffer-16,16);
	if ( (jpg_buffer[jpg_size-2]!=0x0FF) | (jpg_buffer[jpg_size-1]!=0xD9) )
	{
		printf("%d:decode_jpeg() ends: %02X %02X exp:FF D9, not a jpeg\n",
			getpid(),jpg_buffer[jpg_size-2],jpg_buffer[jpg_size-1]);
		fflush(stdout);
		return(JC_ERR);
	}
	if ( (jpg_buffer[0]!=0xFF) | (jpg_buffer[1]!=0xD8) )
	{
		printf("%d:decode_jpeg() got:%02X %02X  exp:FF D8, not a jpeg\n",
		getpid(),jpg_buffer[0],jpg_buffer[1]);
		fflush(stdout);
		return(JC_ERR);
	}

	// Allocate a new decompress struct, with the default error handler.
	// The default error handler will exit() on pretty much any issue,
	// so it's likely you'll want to replace it or supplement it with
	// your own.
	//cinfo.err = jpeg_std_error(&jerr);	
	cinfo.err = jpeg_std_error(&jerr.pub);	
	jerr.pub.error_exit = decodejpeg_error_exit;

	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);

        if (sigsetjmp(jerr.setjmp_buffer,1))
        {
                /* Whoops there was a jpeg error */
                jpeg_destroy_decompress(&cinfo);
                printf("decode_jpeg error handler.\n");
                fflush(stdout);
                return(0);
        }



	// Have the decompressor scan the jpeg header. This won't populate
	// the cinfo struct output fields, but will indicate if the
	// jpeg is valid.
	rc = jpeg_read_header(&cinfo, TRUE);
	if (rc != 1) 
	{
                jpeg_destroy_decompress (&cinfo);
		return(0);
	}

//JA new
	if ((cinfo.image_width>1920) | (cinfo.image_height>1080))
	{
		printf("%d:Decoder has imahe %d x %d, ignoring it\n",getpid(),cinfo.image_width,cinfo.image_height);
                fflush(stdout);
                jpeg_destroy_decompress (&cinfo);
		return(-2);
	}


	// By calling jpeg_start_decompress, you populate cinfo
	// and can then allocate your output bitmap buffers for
	// each scanline.
	jpeg_start_decompress(&cinfo);
	
	bmp_size = cinfo.output_width  * cinfo.output_height * cinfo.output_components;
	*w=cinfo.output_width;
	*h=cinfo.output_height;

	// The row_stride is the total number of bytes it takes to store an
	// entire scanline (row). 
	row_stride = cinfo.output_width * cinfo.output_components;

	//
	// Now that you have the decompressor entirely configured, it's time
	// to read out all of the scanlines of the jpeg.
	//
	// By default, scanlines will come out in RGBRGBRGB...  order, 
	// but this can be changed by setting cinfo.out_color_space
	//
	// jpeg_read_scanlines takes an array of buffers, one for each scanline.
	// Even if you give it a complete set of buffers for the whole image,
	// it will only ever decompress a few lines at a time. For best 
	// performance, you should pass it an array with cinfo.rec_outbuf_height
	// scanline buffers. rec_outbuf_height is typically 1, 2, or 4, and 
	// at the default high quality decompression setting is always 1.
	while (cinfo.output_scanline < cinfo.output_height) 
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return (bmp_size);
}




int write_JPEG_file(char * filename, JSAMPLE * image_buffer, int image_width, int image_height, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile;		/* target file */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  if ((outfile = fopen(filename, "wb")) == NULL) 
	return(1);

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = image_width; 	/* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = image_width * 3;		/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(outfile);
  jpeg_destroy_compress(&cinfo);
  return(0);
}



//https://gist.github.com/royshil/fa98604b01787172b270
//https://stackoverflow.com/questions/32123992/memory-leak-in-jpeg-compression-bug-or-my-mistake

// Jpeg encode RGB data in image_buffer to jpeg data in destbuf
// returns the length of jpeg
int encode_jpeg_tomem(JSAMPLE *image_buffer, unsigned char *destbuf, int image_width, int image_height, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
  unsigned long ol=0;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  unsigned char *imgd = 0;
  jpeg_mem_dest(&cinfo, &imgd, &ol);	// Double pointer oddity, jpeg lib does the malloc

  cinfo.image_width = image_width; 	/* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = image_width * 3;		/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  memcpy(destbuf,imgd,ol);
  free(imgd);
  return(ol);
}





// ******************************************************************************************************************************
// Remove odd or even lines, resulting buffer is half the original height
void deinterlace(int sml, char *data,int width,int height)
{
	int oldbufpos,newbufpos;
	int nlines,npixel;
        char *buffer;
	buffer=data;

//buffer=data;
	//Odd lines kept
	if (sml==1)
		oldbufpos=0;
	else	oldbufpos=width*3;

	newbufpos=0;
	height=height/2;
	for (nlines=1;nlines<=height;nlines++)
	{
		for (npixel=1;npixel<=width;npixel++)
		{
			buffer[newbufpos]  =buffer[oldbufpos];
			buffer[newbufpos+1]=buffer[oldbufpos+1];
			buffer[newbufpos+2]=buffer[oldbufpos+2];
			newbufpos=newbufpos+3;
			oldbufpos=oldbufpos+3;

		}
		oldbufpos=oldbufpos+(3*width);
	}
}


//JA new
void imageflipv(char *imga, int width, int height)
{
	int d;
	int u; 
	unsigned char *buf=NULL;

	buf=malloc(width*height*3);

	u=0;
	for (d=height-1;d>=0;d--)
	{
		memcpy(buf+(d*width*3),imga+(u*(width*3)),width*3);	
		u++;
	}
	memcpy(imga,buf,width*height*3);
	free(buf);
}


void imagefliph(char *imga, int width, int height)
{
	int bpl=width*3;	// byets per line
	int l=0;		// lines
	int y;

	unsigned char *buf=NULL;
	buf=malloc(width*height*3);

	for (l=0;l<height;l++)
	{
		for (y=0;y<width;y++)
		{
			buf[(bpl*l)+(y*3) + 0] = imga[(bpl*l)+bpl-(y*3) + 0];
			buf[(bpl*l)+(y*3) + 1] = imga[(bpl*l)+bpl-(y*3) + 1];
			buf[(bpl*l)+(y*3) + 2] = imga[(bpl*l)+bpl-(y*3) + 2];
		}
	}
	memcpy(imga,buf,width*height*3);
	free(buf);
}




// ******************************************************************************************************************************
// Make the image refernced by *image half its existing width
void halfwidth(char *data,int width,int height,int average)
{
	int oldbufpos,newbufpos;
	int nlines,npixel;
	int origwidth;
        unsigned char *buffer;					// the unsigned matters here

	buffer=(unsigned char*)data;
	//Odd lines kept

	newbufpos=0;
	oldbufpos=0;
	origwidth=width;
	width=width/2;
	for (nlines=1;nlines<=height;nlines++)
	{
		for (npixel=1;npixel<=width;npixel++)
		{
			if (average==TRUE)
			{
				// Average the pixel value as the mean of 2 pixels
				buffer[newbufpos]  = (buffer[oldbufpos]    + buffer[oldbufpos+3]) / 2 ;
				buffer[newbufpos+1]= (buffer[oldbufpos+1]  + buffer[oldbufpos+4]) / 2 ;
				buffer[newbufpos+2]= (buffer[oldbufpos+2]  + buffer[oldbufpos+5]) / 2 ;
				newbufpos=newbufpos+3;
				//Use every other pixel
				oldbufpos=oldbufpos+6;
			} 
			else
			{
				// Dont average at all, just take the leftmost of the two pixels
  				buffer[newbufpos]  =buffer[oldbufpos];
                        	buffer[newbufpos+1]=buffer[oldbufpos+1];
                        	buffer[newbufpos+2]=buffer[oldbufpos+2];
                        	newbufpos=newbufpos+3;
                        	//Use every other pixel
                        	oldbufpos=oldbufpos+6;
			}
		}
		oldbufpos=nlines*3*origwidth;
	}
}



// Scale an image, modify passed width and height
// sc=1 gives 1/4 size
void scaleimage(int sml,char *data, int *w, int *h, int sc)
{
	int i=0;
	int width=*w;	
	int height=*h;

	for (i=0;i<sc;i++)				// do 'sc' times
	{
		deinterlace(sml,data,width,height);
		height=height/2;
		halfwidth(data,width,height,1);
		width=width/2;
	}
	*w = width;
	*h = height;
}



// ******************************************************************************************************************************
/* Modified routine so you can pass colour values. So for example to make a reverse bar across the bottom of the */ 
/* screen pass a string of spaces + a colour value RGB , if rev the last arg is 0 then normal, non zero is shadded block*/
# include "font-6x11.h"
# define CHAR_HEIGHT  11
# define CHAR_WIDTH   6
# define CHAR_START   4
void add_text(char *GRAB_TEXT,char *image, int width, int height, int linesdown, int colsaccross, int R,int G,int B,int rev)
{
    time_t      t;
    struct tm  *tm;
    char        line[128],*ptr; 
    int         i,x,y,f,len;

    // interprete any date/time text in the displayed string
    strcpy(line,GRAB_TEXT);
    len=strlen(line);
    time(&t);
    tm = localtime(&t);
    len = strftime(line,127,GRAB_TEXT,tm);
    //if (debug==1)
        //fprintf(stderr,"%s\n",line);

    int blanklines=(width*3)*linesdown;         // pixels per lines * Numlines * 4 bytes per pixel


    for (y = 0; y < CHAR_HEIGHT; y++) 
    {
        ptr = image + blanklines + (colsaccross*3) + (width * y) * 3;
	for (x = 0; x < len; x++) 
        {
	    f = fontdata6x11[line[x] * CHAR_HEIGHT + y];
	    for (i = CHAR_WIDTH-1; i >= 0; i--) 
	    {
		if (rev==0) 
		{
			if (f & (CHAR_START << i)) 
			{
		    		ptr[0] = R;	/* RGB Bitplanes, was 255 for all */
		    		ptr[1] = G;
		    		ptr[2] = B;
			}
		} else
		{
		    	ptr[0] = R;	/* RGB Bitplanes, was 255 for all */
		    	ptr[1] = G;
		    	ptr[2] = B;
		}
		ptr += 3;
	    }
	}
    }
}



//*********************************************************************************************************************************
// Convert BGR24 (Blue Green Red)  to Red Green Blue
void swap_rgb24(char *mem, int n)
{
    char  c;
    char *p = mem;
    int   i = n;
    
    while (--i) 
    {
	c = p[0]; p[0] = p[2]; p[2] = c;
	p += 3;
    }
}


//*********************************************************************************************************************************
unsigned short int rgbto565(unsigned int R, unsigned int G, unsigned int B)
{
	return (((R>>3) <<11) | ((G>>2) <<5) | (B>>3)); // PCA
}




//*********************************************************************************************************************************
// Plot 24 bit pixel into frame buffer
void plotpixel24(char *framebuffer, int fbwidth, int fbheight, int X, int Y, unsigned char R, unsigned char G, unsigned char B)
{
        unsigned int buffoffset=0;
        int ppl=0;

        ppl=(fbwidth*3)*X;
        buffoffset=ppl+Y*3;							// Skip N lines forward (down the screen)

        if (buffoffset<=(fbwidth*fbheight*3))					// If calculated memory is within framebuffer
        {
                framebuffer[buffoffset+0]=R;
                framebuffer[buffoffset+1]=G;
                framebuffer[buffoffset+2]=B;
        }
}



// populates a buffer with a jpeg containing the RGB value supplied
void jc_RGBsolidcolour(unsigned char *data, int datalen,  unsigned char R,unsigned char G,unsigned char B)
{
	int i=0;

	for (i=0;i<datalen;i=i+3)
	{
		data[i+0]=R;
		data[i+1]=G;
		data[i+2]=B;
	}
}


//int encode_jpeg_tomem(JSAMPLE *image_buffer, unsigned char *destbuf, int image_width, int image_height, int quality)


// From v4l2grab.c
/**
  Convert from YUV422 format to RGB888. Formulae are described on http://en.wikipedia.org/wiki/YUV
  \param width width of image
  \param height height of image
  \param src source
  \param dst destination
*/
void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst)
{
  int line, column;
  unsigned char *py, *pu, *pv;
  unsigned char *tmp = dst;

  /* In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr. 
     Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels. */
  py = src;
  pu = src + 1;
  pv = src + 3;

  #define CLIP(x) ( (x)>=0xFF ? 0xFF : ( (x) <= 0x00 ? 0x00 : (x) ) )

  for (line = 0; line < height; ++line) {
    for (column = 0; column < width; ++column) {
      *tmp++ = CLIP((double)*py + 1.402*((double)*pv-128.0));
      *tmp++ = CLIP((double)*py - 0.344*((double)*pu-128.0) - 0.714*((double)*pv-128.0));
      *tmp++ = CLIP((double)*py + 1.772*((double)*pu-128.0));

      // increase py every time
      py += 2;
      // increase pu,pv every second time
      if ((column & 1)==1) {
        pu += 4;
        pv += 4;
      }
    }
  }
}

