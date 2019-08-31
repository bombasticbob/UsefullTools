//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                 _       _           _      _               _             //
//    _ __  _ __  (_)     | |  ___  __| |    | |_  ___ __  __| |_     ___   //
//   | '__|| '_ \ | |     | | / __|/ _` |    | __|/ _ \\ \/ /| __|   / __|  //
//   | |   | |_) || |     | || (__| (_| |    | |_|  __/ >  < | |_  _| (__   //
//   |_|   | .__/ |_|_____|_| \___|\__,_|_____\__|\___|/_/\_\ \__|(_)\___|  //
//         |_|      |_____|             |_____|                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                 Copyright (c) 2015-2019 by S.F.T. Inc.                   //
//                                                                          //
//    Use, copying, and distribution of this software are licensed with     //
//    GPLv2 or LGPLv2.1, as it is derived from Arduino LiquidCrystal lib    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


// 'rpi_lcd_text' for Raspberry Pi running Raspbian
// This code was derived from Arduino's Liquid Crystal library ([L]GPLv2[.1])
// As derived code it is also covered by the [L]GPLv2.1 (or later)
// For more information, see:  https://github.com/arduino-libraries/LiquidCrystal
// a significant amount of this code has been derived from that githubs project;
// hence this code is also covered by the [L]GPLv2[.1]


// This version uses bit-banging rather than SPI.  To use SPI it needs to be modified and the

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>


// from 'LiquidCrystal' API for Arduino (GPL'd)

#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00


// GPIO pin assignments - SPI0, CS0
#define GPIO_CS_PIN 8
#define GPIO_MOSI_PIN 10
#define GPIO_K_PIN 11

#define GPIO_GID 997 /* the gid for the 'gpio' group, needed on RPi (hard coded) */

// a hack for stringizing defined constants
#define _STRINGIZE(X) #X
#define STRINGIZE(X) _STRINGIZE(X)



void do_startup(void);
void configure_gpio(void);
void send_spi_bits(int bRS, int bEnable, int b1, int b2, int b3, int b4);
void send_text(const char *szText);
void send_command(unsigned char nCmd);
void setCursor(int nCol, int nLine);
void lcd_clear(void);

#define MAX_WIDTH 16
#define MAX_LINES 2

void send_line(int nLine, const char *szText)
{
char tbuf[MAX_WIDTH + 1];

  setCursor(0, nLine);

//  printf("Printing '%s' at 0, %d\n", szText, nLine);

  memset(tbuf, ' ', MAX_WIDTH);
  memcpy(tbuf, szText, strlen(szText));
  tbuf[MAX_WIDTH - 1] = 0;

  send_text(tbuf);
}

void usage(void)
{
  fprintf(stderr, "USAGE:  lcd_text [-n] \"first line\" [\"second line\"]\n");
}

int main(int argc, char *argv[])
{
int bNoStartup = 0;

  while(argc > 1 && argv[1][0] == '-')
  {
    if(argv[1][1]!='n')
    {
      usage();
      return 1;
    }

    // assume '-n'

    bNoStartup = 1;

    argc--;
    argv++;
  }

  if(!bNoStartup)
  {
    do_startup();
  }
//  else
//  {
//    lcd_clear();
//  }

  if(argc > 1)
  {
    send_line(0, argv[1]);
  }
  else
  {
    send_line(0, "");
  }

  if(argc > 2)
  {
    send_line(1, argv[2]);
  }
  else
  {
    send_line(1, "");
  }

  return 0;
}

void output_text_to_FILE(FILE *pF, const char *szText)
{
  fwrite(szText, 1, strlen(szText), pF);
  fflush(pF);
}

void output_text_to_file(const char *szName, const char *szText)
{
FILE *pF = fopen(szName, "w");

  if(!pF)
  {
    fprintf(stderr, "Unable to open %s to send %s", szName, szText);
    return;
  }

  output_text_to_FILE(pF, szText);

  fclose(pF);
}

void output_int_to_FILE(FILE *pF, int iValue)
{
  fprintf(pF, "%d\n", iValue);
  fflush(pF);
}

void output_int_to_file(const char *szName, int iValue)
{
FILE *pF = fopen(szName, "w");

  if(!pF)
  {
    fprintf(stderr, "Unable to open %s to send %d", szName, iValue);
    return;
  }

  output_int_to_FILE(pF, iValue);

  fclose(pF);
}


void configure_gpio_in(int iPin)
{
char tbuf_dir[256];
struct stat st;
int i1;

  sprintf(tbuf_dir, "/sys/class/gpio/gpio%d/direction", iPin);

  while(stat(tbuf_dir, &st) || // both sys files must exist
        !((st.st_mode & S_IRUSR) && (st.st_mode & S_IWUSR) && // owner has r/w rights
           ((st.st_mode & S_IRGRP) && (st.st_mode & S_IWGRP) && st.st_gid == GPIO_GID) || // gpio group access
            (st.st_mode & S_IROTH) && (st.st_mode & S_IWOTH))) // both sys files must exist and be accessible
  {
    if(++i1 > 100) // more than 100 times have I tried
    {
      fprintf(stderr, "unable to access \"%s\", errno=%d (%xH)\n",
              tbuf_dir, errno, errno);
      return;
    }

    output_int_to_file("/sys/class/gpio/export", iPin);
    usleep(10000); // time delay for loop
  }

  output_text_to_file(tbuf_dir, "in");
}

void configure_gpio_out(int iPin, int bValue)
{
char tbuf_dir[256];
char tbuf_val[256];
struct stat st, st2;
int i1;


  sprintf(tbuf_dir, "/sys/class/gpio/gpio%d/direction", iPin);
  sprintf(tbuf_val, "/sys/class/gpio/gpio%d/value", iPin);

  i1 = 0;
  // use 'stat' to check file existence and permissions for gpio control /sys files
  while(stat(tbuf_dir, &st) || stat(tbuf_val, &st2) || // both sys files must exist
        !((st.st_mode & S_IRUSR) && (st.st_mode & S_IWUSR) && // owner has r/w rights
          ((st.st_mode & S_IRGRP) && (st.st_mode & S_IWGRP) && st.st_gid == GPIO_GID) || // gpio group access
           (st.st_mode & S_IROTH) && (st.st_mode & S_IWOTH)) || // any user access
        !((st2.st_mode & S_IRUSR) && (st2.st_mode & S_IWUSR) && // same with the 2nd file
          ((st2.st_mode & S_IRGRP) && (st2.st_mode & S_IWGRP) && st2.st_gid == GPIO_GID) ||
           (st2.st_mode & S_IROTH) && (st2.st_mode & S_IWOTH))) // both sys files must exist and be accessible
  {
    if(++i1 > 100) // more than 100 times have I tried
    {
      fprintf(stderr, "unable to access \"%s\" or\"%s\", errno=%d (%xH)\n",
              tbuf_dir, tbuf_val, errno, errno);
      return;
    }

    output_int_to_file("/sys/class/gpio/export", iPin);
    usleep(10000); // time delay for loop
  }

  output_text_to_file(tbuf_dir, "out");

  // do this anyway whether or not it works... throw it over my shoulder
  output_int_to_file(tbuf_val, bValue);
}

void bit_strobe(FILE *pF10, FILE *pF11, int bVal)
{
  output_int_to_FILE(pF11, 0); // clock low
  output_int_to_FILE(pF10, bVal ? 1 : 0);
  output_int_to_FILE(pF11, 1); // clock high
  output_int_to_FILE(pF11, 0); // clock low
}

void send_spi_bits(int bRS, int bEnable, int b1, int b2, int b3, int b4)
{
static const char szCS[]="/sys/class/gpio/gpio" STRINGIZE(GPIO_CS_PIN) "/value";
static const char szMOSI[]="/sys/class/gpio/gpio" STRINGIZE(GPIO_MOSI_PIN) "/value";
static const char szK[]="/sys/class/gpio/gpio" STRINGIZE(GPIO_K_PIN) "/value";  // this is K for 'K'lock

FILE *pF10 = fopen(szMOSI, "w");
FILE *pF11 = fopen(szK, "w");
FILE *pF25 = fopen(szCS, "w");

  // initial disable
  output_int_to_FILE(pF25, 0);

  // note this prevents values shifted in from being on output

  // clock goes low.  low to high shifts value in

  output_int_to_FILE(pF11, 0);

  bit_strobe(pF10, pF11, 0);
  bit_strobe(pF10, pF11, 0);
  bit_strobe(pF10, pF11, b4);
  bit_strobe(pF10, pF11, b3);
  bit_strobe(pF10, pF11, b2);
  bit_strobe(pF10, pF11, b1);

  bit_strobe(pF10, pF11, bEnable);
  bit_strobe(pF10, pF11, bRS);

  output_int_to_FILE(pF25, 1); // strobe high

  output_int_to_FILE(pF25, 0); // go low again

  fclose(pF25);
  fclose(pF11);
  fclose(pF10);
}

void send_4bits(int bRS, int iBits)
{
int b0, b1, b2, b3;

  b0 = iBits & 1 ? 1 : 0;
  b1 = iBits & 2 ? 1 : 0;
  b2 = iBits & 4 ? 1 : 0;
  b3 = iBits & 8 ? 1 : 0;

//  fprintf(stderr, "iBits=%d, %d %d %d %d\n", iBits, b3, b2, b1, b0);

  send_spi_bits(bRS, 0, b0, b1, b2, b3);  // this sets up the 'RS' pin and bits, and keeps 'enable' low
  send_spi_bits(bRS, 1, b0, b1, b2, b3);  // this sets 'enable' high and leaves the others as-is
  send_spi_bits(bRS, 0, b0, b1, b2, b3);  // this sets 'enable' low and leaves the others as-is

  usleep(37); // settling time to accept the command (37 uS at least)
}

void send_byte(int bRS, unsigned char iVal)
{
  send_4bits(bRS, (iVal >> 4) & 0xf);
  send_4bits(bRS, iVal & 0xf);
}

void send_command(unsigned char iCmd)
{
  send_byte(0, iCmd);
}

void send_data(int iData)
{
  send_byte(1, iData);
}

void send_text(const char *szData)
{
const char *p1;

  p1 = szData;

  while(*p1)
  {
    if(*p1 == '~' && p1[1] == 'n')
    {
      send_data(0xee);
      p1 += 2;
    }
//    else if(*p1 == '`')  TODO: handle accented characters with preceding back-tick?
//    {
//    }

    send_data(*(p1++));
  }
}

void lcd_clear(void)
{
  send_command(LCD_CLEARDISPLAY);
  usleep(2000);
}

void setCursor(int nCol, int nLine)
{
  static unsigned char row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

  if(nLine > 3)
  {
    nLine = 3;
  }

  if(nLine < 0)
  {
    nLine = 0;
  }

  send_command(LCD_SETDDRAMADDR | ((unsigned char)nCol + row_offsets[nLine]));
}

void do_startup(void)
{
  // do strobe first
  configure_gpio_out(GPIO_CS_PIN, 0);

  // next do MOSI, then SCK
  configure_gpio_out(GPIO_MOSI_PIN, 0);
  configure_gpio_out(GPIO_K_PIN, 0);

  // NOTE:  shift reg does rising edge clock when SS is a '1'

  // now start sending some bits.  First I'll clear everything

  send_spi_bits(0,0,0,0,0,0);


  // this next part sets up for 4-bit mode

  send_4bits(0,3);
  usleep(4500);

  send_4bits(0,3);
  usleep(4500);

  send_4bits(0,3);
  usleep(4500);

  send_4bits(0,2);

  // device should be ready to accept 4-bit commands

  send_command(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);


  // display on, cursor off, blink off - command 8 plus 4
  send_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

  // clear the display
  lcd_clear();

  // Initialize to default text direction (for romance languages)
  // LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT  (2)
  // LCD_ENTRYMODESET is 4

  send_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

}

