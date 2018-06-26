// There are 54 gpios in total, arranged in two banks. Bank 1 contains gpios 0-31.  Bank 2 contains gpios 32-54.
// Most of the gpios are dedicated to system use.
// A user should only manipulate gpios in bank 1.
//
//	For a Rev.1 board only use gpios 0, 1, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23, 24, 25.
//	For a Rev.2 board only use gpios 2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27, 28, 29, 30, 31.
//
// It is safe to read all the gpios. If you try to write a system gpio or change its mode you can crash the Pi or corrupt the data on the SD card.
//
//
// Jons notes:
//     Using gpio17 output_low or output_high while SPI is enabled crashes the Pi?              
//		enabling SPI uses GPIO10=SPIO0_MOSI
//				  GPIO9 =SPIO0_MISO
//				  GPIO11=SPIO0_SCLK
//				  GPIO8 =SPIO0_CE0
//				  GPIO7 =SPIO0_CE1
//
//  

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


//#define BCM2708_PERI_BASE        0x20000000				// Pi A,B,B+
#define BCM2708_PERI_BASE        0x3F000000 				// Pi 2, 3, 3+

#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
unsigned char *gpio_mem, *gpio_map;
char *spi0_mem, *spi0_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0
#define GPIO_LEV(g)  (*(gpio+13) >> (g)) & 0x00000001

//new
#define GPIO_READ(g)  *(gpio+13) &= (1<<(g))



// Set a (possibly high) baud rate without linux kernel knowing about it.  Useful for DMX maybe ?
// To get 250k buad you must add "init_uart_clock=16000000" to /boot/config.txt and pass 4,0 to this function
void set_baudrate_divisors(unsigned int divisorwhole, unsigned int divisorpartial)
{
	//*(gpio+201024) =  divisorwhole;
	//*(gpio+201028) =  divisorpartial;

	*(gpio+1024) =  divisorwhole;
	*(gpio+1028) =  divisorpartial;
}




// Set up a memory regions to access GPIO
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) 
   {
      printf("can't open /dev/mem \n");
      exit (-1);
   }

   /* mmap GPIO */

   // Allocate MAP block
   if ((gpio_mem = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) 
   {
      printf("allocation error \n");
      exit (-1);
   }

   // Make sure pointer is on 4K boundary
   if ((unsigned long)gpio_mem % PAGE_SIZE)
     gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);

   // Now map it
   //printf("top=%08X\n",gpio_mem+BLOCK_SIZE);
   gpio_map = (unsigned char *)mmap(
      (caddr_t)gpio_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      GPIO_BASE
   );

   if ((long)gpio_map < 0) 
   {
      printf("mmap error %d\n", (int)gpio_map);
      exit (-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;
} 



void set_as_input(int g)
{
	INP_GPIO(g);
}


// These simplify cut&paste between CCS PIC-C and RPI code.
void set_as_output(int g)
{
    	INP_GPIO(g); 
    	OUT_GPIO(g);
}


void output_high(int g)
{
        GPIO_SET = 1<<g;
}


void output_low(int g)
{
        GPIO_CLR = 1<<g;
}


unsigned int input(int g)
{
	return(GPIO_LEV(g));
}


// Set g to alternate (a=1) or not (a=0)
void set_alternate(int g, int a)
{
	INP_GPIO(g);
	SET_GPIO_ALT(g,a);
}


void delay_us(int usecs)
{
        usleep (usecs);
}

void delay_ms(int msecs)
{
        int i;
        //printf("asked for %d ms\n",msecs); fflush(stdout);

        for (i=0;i<msecs;i++)
                usleep(1000);                                                                   // 1ms
}


