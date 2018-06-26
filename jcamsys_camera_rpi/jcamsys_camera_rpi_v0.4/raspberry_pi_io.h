void set_baudrate_divisors(unsigned int divisorwhole, unsigned int divisorpartial);
void setup_io();
void set_as_input(int g);
void set_as_output(int g);
void output_high(int g);
void output_low(int g);
unsigned int input(int g);
void set_alternate(int g, int a);
void delay_us(int usecs);
void delay_ms(int msecs);


