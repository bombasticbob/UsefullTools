////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//   _               _       _           _      _               _             //
//  | |__   ___   __| |     | |  ___  __| |    | |_  ___ __  __| |_     ___   //
//  | '_ \ / __| / _` |     | | / __|/ _` |    | __|/ _ \\ \/ /| __|   / __|  //
//  | |_) |\__ \| (_| |     | || (__| (_| |    | |_|  __/ >  < | |_  _| (__   //
//  |_.__/ |___/ \__,_|_____|_| \___|\__,_|_____\__|\___|/_/\_\ \__|(_)\___|  //
//                    |_____|             |_____|                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                   Copyright (c) 2018-19 by S.F.T. Inc.                     //
//                                                                            //
//     Use, copying, and distribution of this software are licensed with      //
//     GPLv2 or LGPLv2.1, as it is derived from Arduino LiquidCrystal lib     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


// 'rpi_lcd_text' for Raspberry Pi running Raspbian
// This code was derived from Arduino's Liquid Crystal library ([L]GPLv2[.1])
// As derived code it is also covered by the [L]GPLv2.1 (or later)
// For more information, see:  https://github.com/arduino-libraries/LiquidCrystal
// a significant amount of this code has been derived from that githubs project;
// hence this code is also covered by the [L]GPLv2[.1]
// 'lcd_text.c' for Raspberry Pi - derived from Arduino's Liquid Crystal library ([L]GPLv2.1)
// as derived code it is also covered by the [L]GPLv2.1 (or later) as is all of the Arduino library code

// adapted for FreeBSD as 'bsd_lcd_text.c'

// WIRING INFO    WIRING INFO   WIRING INFO
//
// For this to work, use a standard LCD that's based on the Hitachi HD44780U LCD controller.
// (most of the inexpensive ones with a 15-pin or 16-pin interface seem to be compatible)
//
// For a 16-pin version, you might connect is as shown
//
// ------
// |    | --- 1 -- ground
// |    | --- 2 -- 5V Vcc
// |    | --- 3 -- VD (contrast adjust, 0V to 5V potentiometer)
// |    | --- 4 -- RS - shift register Q1/QA
// |    | --- 5 -- R/W - ground
// | L  | --- 6 -- E - shift register Q2/QB
// |    | --- 7 -- D0 - NC
// | C  | --- 8 -- D1 - NC
// |    | --- 9 -- D2 - NC
// | D  | -- 10 -- D3 - NC
// |    | -- 11 -- D4 - shift register Q3/QC
// |    | -- 12 -- D5 - shift register Q4/QD
// |    | -- 13 -- D6 - shift register Q5/QE
// |    | -- 14 -- D7 - shift register Q6/QF
// |    | -- 15 -- BL+ - back light (47 ohm resistor to 5V)
// |    | -- 16 -- BL- - back light (ground, when present)
// ------
//
// the shift register will typically be a 74HC595, but can also be a 4094 if you wire
// in a transistor to disconnect SCK from the shift reg clock whenever CS is low
//
// The shift reg will otherwise have SCK to clock, MOSI to 'SER' or 'D' (the shift
// reg input), and CS/SS to the 'RCK' (or in the case of 4094, 'strobe') pin.
//
// Other connections on 74HC595 include !G at ground, !SCL at Vcc potential.

#ifdef __FreeBSD__
// TODO:  FBSD-specific interface??

#define USE_SPIGEN /* temporary */
#define USE_SPIGEN_CC /* also temporary */

#define SPI_SPEED 244000

#else // ! __FreeBSD__
#ifdef __linux__

#define USE_SPIDEV
#define SPI_SPEED 244000

#else
#error NOT valid (not linux)
#endif // __linux__
#endif // __FreeBSD__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <inttypes.h>
#include <sys/time.h>


#ifdef USE_SPIDEV
#include <asm-generic/ioctl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#else // USE_SPIDEV
#include <sys/gpio.h>
#ifdef USE_SPIGEN
#include <sys/ioccom.h>
#include <sys/spigenio.h>
#endif // USE_SPIGEN
#endif // USE_SPIDEV


// from 'LiquidCrystal' API for Arduino (probably GPL'd)

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



void do_startup(int hF);
void configure_gpios(int hF);
#ifndef USE_SPIDEV
void bit_strobe(int hF, int iMOSI, int iSCK, int bVal);
int get_pin_caps(int hF, int iPin);
int set_pin_flags(int hF, int iPin, int iFlags);
int set_pin_value(int hF, int iPin, int iVal);
#endif // USE_SPIDEV
void send_spi_byte(int hF, uint8_t bValue);
void send_spi_bits(int hF, int bRS, int bEnable, int b1, int b2, int b3, int b4);
void send_text(int hF, const char *szText);
void send_command(int hF, unsigned char nCmd);
void setCursor(int hF, int nCol, int nLine);
void lcd_clear(int hF);

#define MAX_WIDTH 16
#define MAX_LINES 2

#ifndef USE_SPIDEV
#define SS_PIN   8
#define MOSI_PIN 10
#define SCK_PIN  11
#endif // USE_SPIDEV

void send_line(int hF, int nLine, const char *szText)
{
char tbuf[MAX_WIDTH + 1];

  setCursor(hF, 0, nLine);

//  printf("Printing '%s' at 0, %d\n", szText, nLine);

  memset(tbuf, ' ', MAX_WIDTH);
  memcpy(tbuf, szText, strlen(szText));
  tbuf[MAX_WIDTH - 1] = 0;

  send_text(hF, tbuf);
}

void usage(void)
{
#ifdef USE_SPIGEN
  fprintf(stderr, "USAGE:  bsd_lcd_text [-t] [-n] [-d cs] [-m mode] \"first line\" [\"second line\"]\n");
#else // USE_SPIGEN
  fprintf(stderr, "USAGE:  bsd_lcd_text [-t] [-n] [-m mode] \"first line\" [\"second line\"]\n");
#endif // USE_SPIGEN
}

static int iMode = 0; //SPI_MODE_0;
#ifdef USE_SPIGEN
static int hSPIGEN = -1;
#endif // USE_SPIGEN

int main(int argc, char *argv[])
{
int iDev = 0, bNoStartup = 0, bTestFunc = 0;
int hF;


  while(argc > 1 && argv[1][0] == '-')
  {
    if(argv[1][1] == 't') // test function
    {
      bTestFunc = 1;
      break;
    }
#ifdef USE_SPIGEN
    else if(argv[1][1] == 'd' && argc > 2)
    {
      if(argv[1][2])
      {
        iDev = atoi(&(argv[1][2]));
      }
      else
      {
        argc--;
        argv++;

        iDev = atoi(argv[1]);
      }

      goto end_arg_loop;
    }
#endif // USE_SPIGEN
    else if(argv[1][1] == 'm' && argc > 2)
    {
      if(argv[1][2])
      {
        iMode = atoi(&(argv[1][2]));
      }
      else
      {
        argc--;
        argv++;

        iMode = atoi(argv[1]);
      }

      goto end_arg_loop;
    }
    else if(argv[1][1] == '-') // double-dash
    {
      argc--;
      argc++;
      break; // eat it and quit
    }
    else if(argv[1][1]!='n')
    {
      usage();
      return 1;
    }

    // assume '-n'

    bNoStartup = 1;

end_arg_loop:
    argc--;
    argv++;
  }

//  fprintf(stderr, "TEMPORARY:  mode is %d\n", iMode);

#ifdef USE_SPIDEV
  hF = open("/dev/spidev0.0", O_RDWR); // uses E0 aka GPIO 8 for SS
#else // USE_SPIDEV
  hF = open("/dev/gpioc0", O_RDWR);
#endif // USE_SPIDEV

  if(hF < 0)
  {
    fprintf(stderr, "Unable to open /dev/gpioc0, errno=%d\n", errno);
    return -1;
  }

#ifdef USE_SPIGEN
// TODO:  enumerate spigen devices to find cs=N

  hSPIGEN = open(iDev == 0 ? "/dev/spigen0.0" :
                 iDev == 1 ? "/dev/spigen0.1" :
                 iDev == 2 ? "/dev/spigen0.2" :
                 "/dev/spigen0.0", O_RDWR);
  if(hSPIGEN < 0)
  {
    fprintf(stderr, "Unable to open /dev/spigen0, errno=%d\n", errno);

    close(hF);
    return -1;
  }
#endif // USE_SPIGEN

  if(bTestFunc)
  {
    int iN;

    configure_gpios(hF);

    usleep(1000000);
    fprintf(stderr, "TEST FUNCTION\n");

    for(iN=0; iN < 100; iN++)
    {
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x80);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x40);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x20);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x10);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x8);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x4);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x2);
      usleep(100000);
      fprintf(stderr, ".");
      send_spi_byte(hF, 0x1);
      usleep(100000);
      fprintf(stderr, "\n");
    }

    goto the_end;
  }


  if(!bNoStartup)
  {
    do_startup(hF);
  }
//  else
//  {
//    lcd_clear(hF);
//  }

  if(argc > 1)
  {
    send_line(hF, 0, argv[1]);
  }
  else
  {
    send_line(hF, 0, "");
  }

  if(argc > 2)
  {
    send_line(hF, 1, argv[2]);
  }
  else
  {
    send_line(hF, 1, "");
  }

the_end:

#ifdef USE_SPIGEN
  close(hSPIGEN);
#endif // USE_SPIGEN

  close(hF);

  return 0;
}



/////////////////////////
// GPIO UTILITY FUNCTIONS
/////////////////////////

void wait_microsecond(void)
{
struct timeval tv, tv2;

//  usleep(1); // waits ~1 microsec (TODO:  loop based on CPU freq?)

  gettimeofday(&tv, NULL);

  do
  {
    gettimeofday(&tv2, NULL);
  } while(tv2.tv_usec == tv.tv_usec); // wait for a microsecond to go by

  // do it again so it will truly be at least 1 microsecond (between 1 and 2, realistically)

  do
  {
    gettimeofday(&tv, NULL);
  } while(tv2.tv_usec == tv.tv_usec); // wait for a microsecond to go by
}

void wait_microseconds(int nMicro)
{
#if 0
  usleep(nMicro);
#else // 1
struct timeval tv, tv2;
unsigned long lNew;

  // TODO:  handle nMicro > 1000000

  if(nMicro < 2)
  {
    wait_microsecond();
    return;
  }

  gettimeofday(&tv, NULL);

  lNew = tv.tv_usec + nMicro; // new microsecond value to wait for

  do
  {
    usleep(0);
    gettimeofday(&tv2, NULL);

    if(tv2.tv_sec > tv.tv_sec)
    {
      tv2.tv_usec += 1000000; // only allow for 1 second 'wraparound'
    }
  } while(tv2.tv_usec < lNew); // wait for a microsecond to go by
#endif // 0
}

#ifndef USE_SPIDEV
void configure_gpio_in(int hF, int iPin)
{
  if(set_pin_flags(hF, iPin, GPIO_PIN_INPUT) < 0)
  {
    fprintf(stderr, "ERROR:  set_pin_flags %d GPIO_PIN_INPUT\n", iPin);
  }
}

void configure_gpio_out(int hF, int iPin, int bValue)
{
  if(set_pin_flags(hF, iPin, GPIO_PIN_OUTPUT) < 0)
  {
    fprintf(stderr, "ERROR:  set_pin_flags %d GPIO_PIN_OUTPUT\n", iPin);
  }

  set_pin_value(hF, iPin, bValue ? 1 : 0); // this will already print an error if it fails
}
#endif // USE_SPIDEV

void configure_gpios(int hF)
{
#ifdef USE_SPIDEV

  int spiBPW = 8;
  int speed = SPI_SPEED;

  if(ioctl(hF, SPI_IOC_WR_MODE, &iMode /*&mode*/) < 0)
  {
    fprintf(stderr, "ioctl error SPI_IOC_WR_MODE errno=%d\n", errno);
  }
  if(ioctl(hF, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) < 0)
  {
    fprintf(stderr, "ioctl error SPI_IOC_WR_BITS_PER_WORD errno=%d\n", errno);
  }
  if(ioctl(hF, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
  {
    fprintf(stderr, "ioctl error SPI_IOC_WR_MAX_SPEED_HZ errno=%d\n", errno);
  }

  send_spi_byte(hF, 0);

#else

#ifndef USE_SPIGEN_CC
  // do strobe first, set to 1
  configure_gpio_out(hF, SS_PIN, 1);
#endif // USE_SPIGEN_CC

#ifdef USE_SPIGEN

  uint32_t dwClockSpeed = SPI_SPEED;

//  fprintf(stderr, "TEMPORARY:  caps  MOSI:  %xH   SCK:  %xH\n",
//          get_pin_caps(hF, MOSI_PIN),
//          get_pin_caps(hF, SCK_PIN));


//  set_pin_flags(hF, MOSI_PIN, 0);
//  set_pin_flags(hF, SCK_PIN, 0);

  if(ioctl(hSPIGEN, SPIGENIOC_SET_CLOCK_SPEED, &dwClockSpeed) < 0)
  {
    fprintf(stderr, "ioctl error SPIGENIOC_SET_CLOCK_SPEED errno=%d\n", errno);
  }

#else // USE_SPIGEN

  // next do MOSI, then SCK
  configure_gpio_out(hF, MOSI_PIN, 0);
  configure_gpio_out(hF, SCK_PIN, 0);

#endif // USE_SPIGEN

  // NOTE:  shift reg does rising edge clock when SS is a '1'

  // now start sending some bits.  First I'll clear everything

  send_spi_byte(hF, 0);
//  send_spi_bits(hF, 0,0,0,0,0,0);

#endif

  wait_microseconds(100);
}


#ifndef USE_SPIDEV
int get_pin_caps(int hF, int iPin)
{
int iRet;
struct gpio_pin xPin;


  bzero(&xPin, sizeof(xPin));
  xPin.gp_pin = iPin;

  iRet = ioctl(hF, GPIOGETCONFIG, &xPin);

  if(iRet < 0)
  {
    fprintf(stderr, "ERROR:  ioctl fail, GPIOGETCONFIG, errno=%d\n", errno);
    return -1;
  }

  return xPin.gp_caps;
}

int set_pin_flags(int hF, int iPin, int iFlags)
{
int iRet, iCaps;
struct gpio_pin xPin;


  bzero(&xPin, sizeof(xPin));
  xPin.gp_pin = iPin;
  xPin.gp_flags = iFlags;

  iCaps = get_pin_caps(hF, iPin);

  if((iFlags & iCaps) != iFlags) // flags specified that aren't capable?
  {
    fprintf(stderr, "ERROR:  ioctl caps do not match  pin=%d, caps=%04xH, flags=%04xH\n",
            iPin, iCaps, iFlags);

    return -1;
  }

  iRet = ioctl(hF, GPIOSETCONFIG, &xPin);

  if(iRet < 0)
  {
    fprintf(stderr, "ERROR:  ioctl fail, GPIOSETCONFIG, errno=%d  pin=%d, flags=%04xH\n", errno, iPin, iFlags);

    return -1;
  }

  return 0;
}

int set_pin_value(int hF, int iPin, int iVal)
{
int iRet;
struct gpio_req xReq;

  bzero(&xReq, sizeof(xReq));
  xReq.gp_pin = iPin;
  xReq.gp_value = iVal;

  iRet = ioctl(hF, GPIOSET, &xReq);

  if(iRet < 0)
  {
    fprintf(stderr, "ERROR:  ioctl fail, GPIOSET, errno=%d\n", errno);
    return -1;
  }

  return 0; // success!
}

void bit_strobe(int hF, int iMOSI, int iSCK, int bVal)
{
  set_pin_value(hF, iSCK, 0); // make sure
  wait_microsecond();

  set_pin_value(hF, iMOSI, bVal ? 1 : 0);
  wait_microsecond();

  set_pin_value(hF, iSCK, 1);
  wait_microsecond();

  set_pin_value(hF, iSCK, 0);
}
#endif // USE_SPIDEV

void send_spi_byte(int hF, uint8_t bValue)
{
#ifdef USE_SPIDEV
struct spi_ioc_transfer spi ;
static uint8_t data[2];

  data[0] = bValue;
  data[1] = bValue;

  bzero(&spi, sizeof (spi));

  spi.tx_buf        = (uintptr_t)&(data[0]);
  spi.rx_buf        = (uintptr_t)&(data[1]);
  spi.len           = 1;
  spi.cs_change     = 0;
  spi.delay_usecs   = 0;
  spi.speed_hz      = SPI_SPEED;
  spi.bits_per_word = 8;

  if(ioctl(hF, SPI_IOC_MESSAGE(1), &spi) < 0)
  {
    fprintf(stderr, "ioctl error SPI_IOC_MESSAGE errno=%d\n", errno);
  }

  wait_microsecond();

#elif defined(USE_SPIGEN)
struct spigen_transfer spi;
uint8_t aData[2];

  aData[0] = bValue;

#ifndef USE_SPIGEN_CC
  set_pin_value(hF, SS_PIN, 0);
  wait_microseconds(10);
#endif // USE_SPIGEN_CC

  bzero(&spi, sizeof(spi));
  spi.st_command.iov_base = &(aData[0]);
  spi.st_command.iov_len = 1;
  spi.st_data.iov_base = NULL;
  spi.st_data.iov_len = 0;

  if(ioctl(hSPIGEN, SPIGENIOC_TRANSFER, &spi) < 0)
  {
    fprintf(stderr, "ioctl error SPIGENIOC_TRANSFER errno=%d\n", errno);
  }

#ifndef USE_SPIGEN_CC
  wait_microsecond();
  set_pin_value(hF, SS_PIN, 1); // strobe high (for shift reg, not SPI)
#endif // USE_SPIGEN_CC

  wait_microsecond();

#else // USE_SPIDEV
int nB = 0x80;

  // initial disable, allows shifting bits in
  set_pin_value(hF, SCK_PIN, 0);
  set_pin_value(hF, SS_PIN, 0);

  while(nB > 0)
  {
    bit_strobe(hF, MOSI_PIN, SCK_PIN, (bValue & nB) ? 1 : 0);

    nB = nB >> 1;
  }

  set_pin_value(hF, SS_PIN, 1); // strobe high (for shift reg, not SPI)
  wait_microsecond();

  set_pin_value(hF, SCK_PIN, 0); // make sure
#endif // USE_SPIDEV
}

void send_spi_bits(int hF, int bRS, int bEnable, int b1, int b2, int b3, int b4)
{
uint8_t bByte;

#if !defined(USE_SPIDEV) && !defined(USE_SPIGEN_CC)

  // initial disable, allows shifting bits in
  set_pin_value(hF, SS_PIN, 0);

  // note this prevents values shifted in from being on output

  // clock goes low.  low to high shifts value in

  set_pin_value(hF, SCK_PIN, 0);
#endif // USE_SPIDEV, USE_SPIGEN_CC

  bByte = 0;
  if(b4)
  {
    bByte |= 0x20;
  }
  if(b3)
  {
    bByte |= 0x10;
  }
  if(b2)
  {
    bByte |= 0x8;
  }
  if(b1)
  {
    bByte |= 0x4;
  }

  if(bEnable)
  {
    bByte |= 0x2;
  }
  if(bRS)
  {
    bByte |= 0x1;
  }

  send_spi_byte(hF, bByte);

#if !defined(USE_SPIDEV) && !defined(USE_SPIGEN_CC)
  set_pin_value(hF, SS_PIN, 1); // strobe high (shift reg will transfer content)
  wait_microsecond();
  set_pin_value(hF, SCK_PIN, 0); // make sure
#endif // USE_SPIDEV, USE_SPIGEN_CC
}



////////////////////////
// LCD UTILITY FUNCTIONS
////////////////////////

void send_4bits(int hF, int bRS, int iBits)
{
int b0, b1, b2, b3;

  b0 = iBits & 1 ? 1 : 0;
  b1 = iBits & 2 ? 1 : 0;
  b2 = iBits & 4 ? 1 : 0;
  b3 = iBits & 8 ? 1 : 0;

  send_spi_bits(hF, bRS, 0, b0, b1, b2, b3);  // this sets up the 'RS' pin and bits, and keeps 'enable' low
  wait_microsecond();
  send_spi_bits(hF, bRS, 1, b0, b1, b2, b3);  // this sets 'enable' high and leaves the others as-is
  wait_microsecond();
  send_spi_bits(hF, bRS, 0, b0, b1, b2, b3);  // this sets 'enable' low and leaves the others as-is

  //wait_microseconds(37);
  usleep(37); // settling time to accept the command (37 uS at least)
}

void send_byte(int hF, int bRS, unsigned char iVal)
{
  send_4bits(hF, bRS, (iVal >> 4) & 0xf);
  send_4bits(hF, bRS, iVal & 0xf);
}

void send_command(int hF, unsigned char iCmd)
{
//  fprintf(stderr, "Send Command:  %d (%xH)\n", iCmd, iCmd);
  send_byte(hF, 0, iCmd);
}

void send_data(int hF, int iData)
{
//  fprintf(stderr, "Send Data:  %d (%xH)\n", iData, iData);
  send_byte(hF, 1, iData);
}

void send_text(int hF, const char *szData)
{
const char *p1;

  p1 = szData;

  while(*p1)
  {
    if(*p1 == '~' && p1[1] == 'n')
    {
      send_data(hF, 0xee);
      p1 += 2;
    }
//    else if(*p1 == '`')  TODO: handle accented characters with preceding back-tick?
//    {
//    }

    send_data(hF, *(p1++));
  }
}

void lcd_clear(int hF)
{
  send_command(hF, LCD_CLEARDISPLAY);
  usleep(2000);
}

void setCursor(int hF, int nCol, int nLine)
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

  send_command(hF, LCD_SETDDRAMADDR | ((unsigned char)nCol + row_offsets[nLine]));
}

void do_startup(int hF)
{
  configure_gpios(hF);

  // this next part sets up for 4-bit mode

  send_4bits(hF, 0,3);
  usleep(4500);

  send_4bits(hF, 0,3);
  usleep(4500);

  send_4bits(hF, 0,3);
  usleep(4500);

  send_4bits(hF, 0,2);

  // device should be ready to accept 4-bit commands

  send_command(hF, LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS);


  // display on, cursor off, blink off - command 8 plus 4
  send_command(hF, LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

  // clear the display
  lcd_clear(hF);

  // Initialize to default text direction (for romance languages)
  // LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT  (2)
  // LCD_ENTRYMODESET is 4

  send_command(hF, LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

}

