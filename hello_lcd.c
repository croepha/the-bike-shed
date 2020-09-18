
#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock


static const int LCD_D4 = 25;
static const int LCD_D5 = 24;
static const int LCD_D6 = 23;
static const int LCD_D7 = 18;



static const int LCD_RS = 7;
static const int LCD_E  = 8;
static const int LCD_WIDTH = 20;
static const int LCD_CHR = 1;
static const int LCD_CMD = 0;
static const int LCD_LINE_1 = 0x80;
static const int LCD_LINE_2 = 0xC0;
static const int LCD_LINE_3 = 0x94;
static const int LCD_LINE_4 = 0xD4;
static const long E_PULSEus = 500;
static const long E_DELAYus = 500;

static const int True = 1;
static const int False = 0;
static const int GPIO_out = 123;

static void time_sleep_us(long us) { usleep(us); }

static void GPIO_output(int pin, int value) {
    if (value) {
        GPIO_SET = 1<<pin;
    } else {
        GPIO_CLR = 1<<pin;
    }
}

// void __t(int bits2, int n) {
//   int bit_mask = 1<<data_pins[n];
//   GPIO_CLR = bit_mask & 0xffffffff;
//   int v01 = !!(bits2&(1<<n));
//   GPIO_SET = bit_mask & v01 << data_pins[n];
// }

// void __t(int bits2, int n) {
//   GPIO_CLR = 1 << data_pins[n];
//   int v1 = 0;
//   if ((bits2&(1<<n))==(1<<n)) {
//       GPIO_SET = 1 << data_pins[n];
//       v1 = 1 << data_pins[n];
//   }
//   assert(v1 == (((bits2&(1<<n))==(1<<n)) << data_pins[n]));

//   // int t = (bits2&(1<<n))==(1<<n);
//   // if (t) {
//   //     v1 = 1 << data_pins[n];
//   //     GPIO_SET = v1;
//   // }
//   // assert(v1 == (t << data_pins[n]));

// }


// void __t(int bits2, int n) {
//   GPIO_output(data_pins[n], (bits2&(1<<n))==(1<<n));
// }


static void GPIO_setup(int pin, int mode) {
    assert(mode == GPIO_out);
    INP_GPIO(pin); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(pin);

}

static void lcd_toggle_enable() {
  // Toggle enable
  time_sleep_us(E_DELAYus);
  GPIO_output(LCD_E, True);
  time_sleep_us(E_PULSEus);
  GPIO_output(LCD_E, False);
  time_sleep_us(E_DELAYus);
}

static int const data_pins[] = {
  LCD_D4, LCD_D5, LCD_D6, LCD_D7
};

static int abit(int bits2, int n) {
  int v01 = !!(bits2&(1<<n));
  return v01 << data_pins[n];
}


const int bits_mask =
  1 << LCD_D4 |
  1 << LCD_D5 |
  1 << LCD_D6 |
  1 << LCD_D7;

static void send4bits(int bits) {
  #define _t(n) bits_values = bits_values | abit(bits, n)
  int bits_values = 0;
  _t(0); _t(1); _t(2); _t(3);
  GPIO_CLR = bits_mask& 0xFFFFFFFF;
  GPIO_SET = bits_mask& bits_values;
}


static void lcd_byte(int bits, int mode) {
  // Send byte to data pins
  // bits = data
  // mode = True  for character
  //        False for command

  GPIO_output(LCD_RS, mode); // RS

//  GPIO_CLR = bits_mask&~bits_values;

//  #define _t(n) bits_values = bits_values | ( (!!(bits2&(1<<n))) << (data_pins[n]) )
//    int bits_values;

//   int bits2 = bits >> 4;
//   bits_values = 0;
//   _t(0); _t(1); _t(2); _t(3);
//   GPIO_CLR = bits_mask& 0xFFFFFFFF;
//   GPIO_SET = bits_mask& bits_values;
//   lcd_toggle_enable();
//   bits2 = bits;
//   bits_values = 0;
//   _t(0); _t(1); _t(2); _t(3);
//   GPIO_CLR = bits_mask& 0xFFFFFFFF;
//   GPIO_SET = bits_mask& bits_values;
//   lcd_toggle_enable();


  //  GPIO_CLR = bits_mask&~bits_values;
  //  GPIO_SET = bits_mask& bits_values;

//#define _t(n) __t(bits2, n)
//#define _t(n) GPIO_output(data_pins[n], (bits2&(1<<n))==(1<<n))
  int bits2 = bits >> 4;
  send4bits(bits2);
//  _t(0); _t(1); _t(2); _t(3);
  lcd_toggle_enable();
  bits2 = bits;
  send4bits(bits2);
  lcd_toggle_enable();



// #define _t(n)  if ((bits&(1<<n))==(1<<n))

//   GPIO_output(LCD_D4, False);
//   GPIO_output(LCD_D5, False);
//   GPIO_output(LCD_D6, False);
//   GPIO_output(LCD_D7, False);
//   _t(4)
//     GPIO_output(LCD_D4, True);
//   _t(5)
//     GPIO_output(LCD_D5, True);
//   _t(6)
//     GPIO_output(LCD_D6, True);
//   _t(7)
//     GPIO_output(LCD_D7, True);

//   // Toggle 'Enable' pin
//   lcd_toggle_enable();

//   // Low bits
//   GPIO_output(LCD_D4, False);
//   GPIO_output(LCD_D5, False);
//   GPIO_output(LCD_D6, False);
//   GPIO_output(LCD_D7, False);
//   _t(0)
//     GPIO_output(LCD_D4, True);
//   _t(1)
//     GPIO_output(LCD_D5, True);
//   _t(2)
//     GPIO_output(LCD_D6, True);
//   _t(3)
//     GPIO_output(LCD_D7, True);

//   // Toggle 'Enable' pin
//  lcd_toggle_enable();

}

static void lcd_init() {
  // Initialise display
  lcd_byte(0x33,LCD_CMD); // 0011 0011 Initialise
  lcd_byte(0x32,LCD_CMD); // 0011 0010 Initialise
  lcd_byte(0x06,LCD_CMD); // 0000 0110 Cursor move direction
  lcd_byte(0x0C,LCD_CMD); // 0000 1100 Display On,Cursor Off, Blink Off
  lcd_byte(0x28,LCD_CMD); // 0010 1000 Data length, number of lines, font size
  lcd_byte(0x01,LCD_CMD); // 0000 0001 Clear display
  time_sleep_us(E_DELAYus);
}

static void lcd_string(char*message, int line) {
  lcd_byte(line, LCD_CMD);

  for (int i=0; i<LCD_WIDTH && message[i]; i++) {
    lcd_byte(message[i],LCD_CHR);
  }
}

void setup_io();

int main() {
  // Main program block
  setup_io();

  GPIO_setup(LCD_E, GPIO_out);
  GPIO_setup(LCD_RS, GPIO_out);
  GPIO_setup(LCD_D4, GPIO_out);
  GPIO_setup(LCD_D5, GPIO_out);
  GPIO_setup(LCD_D6, GPIO_out);
  GPIO_setup(LCD_D7, GPIO_out);

  // Initialise display
  lcd_init();

  while(1) {

    // Blank display
    lcd_byte(0x01, LCD_CMD);

    // Send some centred test
    lcd_string("--------------------",LCD_LINE_1);
    lcd_string("Rasbperry Pi",LCD_LINE_2);
    lcd_string("Model B",LCD_LINE_3);
    lcd_string("--------------------",LCD_LINE_4);

    time_sleep_us(1000000); // 3 second delay

    // Blank display
    lcd_byte(0x01, LCD_CMD);

    lcd_string("Raspberrypi-spy",LCD_LINE_1);
    lcd_string(".co.uk",LCD_LINE_2);
    lcd_string("",LCD_LINE_3);
    lcd_string("20x4 LCD Module Test",LCD_LINE_4);

    time_sleep_us(1000000); // 20 second delay


  }
}



//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;


} // setup_io
