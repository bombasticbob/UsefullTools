#!/bin/sh
#
# read.fuses.sh - Copyright 2019 by Big Bad Bombastic Bob
#
# released to public domain - enjoy!
# (I did a lot of work here, it's probably educational)
#
# this utility assumes that avrdude is in /usr/local/bin and that its
# config file is in /usr/local/etc/avrdude.conf - if your system is different
# just edit the appropriate stuff.

# convert 1st param to a binary value
to_binary ( )
{
  xx=`echo $1 | awk '{ print $1 + 0; }'`
  if test $xx -eq 0 ; then
    rr="0"
  else
    rr=""
    while test $xx -gt 0 ; do
      bb=`expr $xx % 2`
      xx=`expr $xx / 2`
      rr=${bb}${rr}
    done
  fi

  echo $rr | awk '{ AA="000000000" $1; print substr(AA,length(AA)-7,8); }'
}

# 1st param is value, 2nd param is bitmask
is_bit_set ( )
{
  zz=`to_binary $1`
  yy=`to_binary $2`

  rr=`echo $zz $yy | awk '{ RR=1; if(length($1) != length($2)) print "0"; else { for(II=1; II <= length($1); II++) { if(substr($1,II,1) == 0 && substr($2,II,1) != 0) RR=0; } print RR; }}'`

  printf "%d" $rr
}

# print out the info for external clock and similar ones
do_external_clock_normal ( )
{
  case $1 in
    (00) echo "Fast rising power or BOD enabled, 6 clocks + 14clocks" ;;
    (01) echo "Slowly rising power, 6 clocks + 14clocks + 4.1 msec";;
    (10) echo "Stable freq at startup, 6 clocks + 14clocks + 65msec";;
    (11) echo "SUT 11 - reserved";;
    (*) echo "unrecognized value $1";;
  esac
}


if test -z "$1" ; then
  echo Using default CPU type atmega328p
  CPU=atmega328p
else
  CPU="$1"
fi

case $CPU in
  (*mega328*) CPUTYPE=328;;
  (*mega168*) CPUTYPE=168;;
  (*mega88*) CPUTYPE=88;;
  (*mega48*) CPUTYPE=48;;
  (*) echo "invalid CPU type $CPU"; exit;;
esac

# the AVRDUDE command - yes the paths are hard coded.  deal with it
/usr/local/bin/avrdude -F -cavrispmkII -Pusb -v -C/usr/local/etc/avrdude.conf \
  -p${CPU} \
  -Ulock:r:/tmp/outX.out:h -Uefuse:r:/tmp/outE.out:h -Uhfuse:r:/tmp/outH.out:h -Ulfuse:r:/tmp/outL.out:h

# SOME DOCUMENTATION STARTS HERE
# ------------------------------
#
# fuse info:
# a) '0' means 'programmed', '1' means 'not programmed' - data sheet section 9.2
# b) CKSEL preprogrammed to 0010 (8mhz internal)
# c) CKDIV8 is pre-programmed (aka '0') to give 1mhz clock
# d) SUT is pre-programmed to '10' (max startup time)
#
# low freq crystal oscillator (100x through 111x) bit settings
#
# truth table for CKSEL bits 3..1 - tables 9-1 and 9-3
# --------------------------------------------------------------------------------
#  value  meaning
#   0000  external clock
#   0001  reserved
#   0010  Calibrated internal RC oscillator (8mhz)
#   0011  Internal 128khz oscillator
#   010x  Low Freq crystal oscillator
#   011x  Full Swing crystal oscillator
#   100x  Low power crystal osc (0.4 to 0.9 Mhz, resonator only)
#   101x  Low power crystal osc (0.9 to 3.0 Mhz)
#   110x  Low power crystal osc (3.0 to 8.0 Mhz)
#   111x  Low power crystal osc (8.0 to 16.0 Mhz)
#
#  Low power and Full Swing crystal osc, CKSEL bit 0 plus SUT - tables 9-4 and 9-6
# --------------------------------------------------------------------------------
#  bit 0  SUT   meaning
#   xxx0  00    ceramic resonator, fast rising power, 258 clocks plus 14 clocks plus additional [4.1ms]
#   xxx0  01    ceramic resonator, slowly rising power, 258 clocks plus 14 clocks plus additional [65ms]
#   xxx0  10    ceramic resonator, BOD enabled, 1k clocks plus 14 clocks
#   xxx0  11    ceramic resonator, fast rising power, 1k clocks plus 14 clocks plus additional [4.1ms]
#   xxx1  00    ceramic resonator, slowly rising power, 1k clocks plus 14 clocks plus additional [65ms]
#   xxx1  01    crystal oscillator, BOD enabled, 16k clocks + 14 clocks
#   xxx1  10    crystal oscillator, fast rising power, 16k clocks plus 14 clocks plus additional [4.1ms]
#   xxx1  11    crystal oscillator, slowly rising power, 16k clocks plus 14 clocks plus additional [65ms]
#
#
#  CKSEL bit 0 and SUT bits for low freq crystal oscillator
# --------------------------------------------------------------------------------
#  CKSEL  meaning
#   0100  1k clocks startup from power down/power save
#   0101  32k clocks startup from power down/power save
#
#  SUT   meaning
#   00   Fast rising power or BOD enabled, 4 clocks
#   01   Slowly rising power, 4 clocks + 4.1 msec
#   10   Stable freq at startup, 4 clocks + 65msec
#   11   reserved
#
#
#  SUT bits for internal RC osc and 128k osc and external
# --------------------------------------------------------------------------------
#  SUT   meaning
#   00   Fast rising power or BOD enabled, 6 clocks + 14clocks
#   01   Slowly rising power, 6 clocks + 14clocks + 4.1 msec
#   10   Stable freq at startup, 6 clocks + 14clocks + 65msec
#   11   reserved
#
#
#  BOD fuses
# --------------------------------------------------------------------------------
# value  meaning
#  111   disabled
#  110   min = 1.7, typ = 1.8, max = 2.0
#  101   min = 2.5, typ = 2.7, max = 2.9
#  100   min = 4.1, typ = 4.3, max = 4.5
# (others reserved)
#
#
# BOOTRST bit
# --------------------------------------------------
#  0  boot loader reset vector (see table 27-7)
#  1  application reset vector (0000)
#
#
# BOOTSZx FLAGS
#
#  BOOTSZ1,BOOTSZ0  meaning  (27-7, 27-10)  mega88   mega168
#    11             128 words (4 pages)      F80       1F80
#    10             256 words (8 pages)      F00       1F00
#    01             512 words (16 pages)     E00       1E00
#    00             1024 words (32 pages)    C00       1C00
#
#  BOOTSZ1,BOOTSZ0  meaning  (27-7, 27-10)  mega328
#    11             256 words (4 pages)       3F00
#    10             512 words (8 pages)       3E00
#    01             1024 words (16 pages)     3C00
#    00             2048 words (32 pages)     3800
#
#
# ACTUAL FUSE BIT LAYOUTS (different for each CPU type)
#
# lock bits - see 27-2 and 27.8.7 and 28-1
#   LOCK BIT REGISTER:  | NA | NA | BLB12 | BLB11 | BLB02 | BLB01 | LB2 | LB1 |
#   a '1' is "unprogrammed" (means unlocked)
#
#   for LB1 and LB2, see 28-2
#   in essence, LB2,LB1 have values 11 (unlocked), 10 (lock EEPROM+NVRAM), 00 (lock fuses also)
#
#   for BLBxx see 28-3 - BLB0x locks app section, BLB1x locks boot section
#     11 = unlocked, 10 lock SPM writes, 00 lock read/write from other section, 01 only lock read from other section (write OK)
#
#
# EXTENDED FLAGS  - see tables 28-4 and 28-5, 28-6
#
#   on the '48' bit 0 'SELFPROGEN' determines if self-programming is enabled
#         | NA | NA | NA | NA | NA | NA | NA | SELFPROGEN |
#
#     SELFPROGEN = Self Programming Enabled (0 to enable, 1 to disable)
#
#   for mega88, mega168
#         | NA | NA | NA |  NA | NA | BOOTSZ1 | BOOTSZ0 | BOOTRST |
#
#   for the mega328:
#         | NA | NA | NA |  NA | NA | BODLEVEL2 | BODLEVEL1 | BODLEVEL0 |
#
#
# FUSE HIGH BYTE
#
# for mega48, mega88, mega168
#         | RTDISBL | DWEN | SPIEN | WDTON | EESAVE | BODLEVEL2 | BODLEVEL1 | BODLEVEL0 |
#
# for mega328
#         | RTDISBL | DWEN | SPIEN | WDTON | EESAVE | BOOTSZ1 | BOOTSZ0 | BOOTRST |
#
#
# FUSE LOW BYTE
# for all
#         | CKDIV8 | CKOUT | SUT1 | SUT0 | CKSEL3 | CKSEL2 | CKSEL1 | CKSEL0 |
#

# now to decode all of that

FUSEL=`cat /tmp/outL.out`
FUSEH=`cat /tmp/outH.out`
FUSEE=`cat /tmp/outE.out`
LOCKBITS=`cat /tmp/outX.out`

CKDIV8=`is_bit_set $FUSEL 128`
CKOUT=`is_bit_set $FUSEL 64`
SUT1=`is_bit_set $FUSEL 32`
SUT0=`is_bit_set $FUSEL 16`
CKSEL3=`is_bit_set $FUSEL 8`
CKSEL2=`is_bit_set $FUSEL 4`
CKSEL1=`is_bit_set $FUSEL 2`
CKSEL0=`is_bit_set $FUSEL 1`

RTDISBL=`is_bit_set $FUSEH 128`
DWEN=`is_bit_set $FUSEH 64`
SPIEN=`is_bit_set $FUSEH 32`
WDTON=`is_bit_set $FUSEH 16`
EESAVE=`is_bit_set $FUSEH 8`

if test $CPUTYPE -eq 328 ; then
  # for 328, boot size is in FUSEH
  BOOTSZ1=`is_bit_set $FUSEH 4`
  BOOTSZ0=`is_bit_set $FUSEH 2`
  BOOTRST=`is_bit_set $FUSEH 1`
else
  BODLEVEL2=`is_bit_set $FUSEH 4`
  BODLEVEL1=`is_bit_set $FUSEH 2`
  BODLEVEL0=`is_bit_set $FUSEH 1`
fi

if test $CPUTYPE -eq 48 ; then
  SELFPROGEN=`is_bit_set $FUSEE 1`
elif test $CPUTYPE -ne 328 ; then
  BOOTSZ1=`is_bit_set $FUSEE 4`
  BOOTSZ0=`is_bit_set $FUSEE 2`
  BOOTRST=`is_bit_set $FUSEE 1`
else
  # for 328, BOD level is in FUSEE
  BODLEVEL2=`is_bit_set $FUSEE 4`
  BODLEVEL1=`is_bit_set $FUSEE 2`
  BODLEVEL0=`is_bit_set $FUSEE 1`
fi

#   LOCK BIT REGISTER:  | NA | NA | BLB12 | BLB11 | BLB02 | BLB01 | LB2 | LB1 |
BLB12=`is_bit_set $LOCKBITS 32`
BLB11=`is_bit_set $LOCKBITS 16`
BLB02=`is_bit_set $LOCKBITS 8`
BLB01=`is_bit_set $LOCKBITS 4`
LB2=`is_bit_set $LOCKBITS 2`
LB1=`is_bit_set $LOCKBITS 1`

echo "RAW FUSE VALUES:"
echo "  lock bits:      " `to_binary $LOCKBITS`
echo "  extended bits:  " `to_binary $FUSEE`
echo "  high bits:      " `to_binary $FUSEH`
echo "  low bits:       " `to_binary $FUSEL`
echo ""
echo "LOCK BITS"
echo "  BLB12    " `if test "$BLB12" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo "  BLB11    " `if test "$BLB11" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo "  BLB02    " `if test "$BLB02" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo "  BLB01    " `if test "$BLB01" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo "  LB2      " `if test "$LB2" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo "  LB1      " `if test "$LB1" -eq 1 ; then echo "UNLOCKED" ; else echo "LOCKED" ; fi`
echo ""
echo "SETTINGS"
echo "  CKDIV8:  " `if test "$CKDIV8" -eq 1 ; then echo "OFF" '(clock 1:1)'; else echo "ON" '(clock divide by 8)' ; fi`
echo "  CKOUT:   " `if test "$CKOUT" -eq 1 ; then echo "OFF" '(no clock output)' ; else echo "ON" '(clock output on PORTB0)' ; fi`
echo "  RTDISBL: " `if test "$RTDISBL" -eq 1 ; then echo "OFF" '(external reset enabled)' ; else echo "ON" '(disable external reset)' ; fi`
echo "  DWEN:    " `if test "$DWEN" -eq 1 ; then echo "OFF" '(debug WIRE disabled)'; else echo "ON" '(debug WIRE enabled)' ; fi`
echo "  SPIEN:   " `if test "$SPIEN" -eq 1 ; then echo "OFF" '(SPI programming DISabled)' ; else echo "ON" '(SPI programming enabled)' ; fi`
echo "  WDTON:   " `if test "$WDTON" -eq 1 ; then echo "OFF" '(WDT not always on)' ; else echo "ON" '(WDT always on)' ; fi`
echo "  EESAVE:  " `if test "$EESAVE" -eq 1 ; then echo "OFF" '(chip erase includes EEPROM)' ; else echo "ON" '(chip erase excludes EEPROM)'; fi`
echo ""
echo "BOD (brownout detect) LEVEL"
#  111   disabled
#  110   min = 1.7, typ = 1.8, max = 2.0
#  101   min = 2.5, typ = 2.7, max = 2.9
#  100   min = 4.1, typ = 4.3, max = 4.5
case ${BODLEVEL2}${BODLEVEL1}${BODLEVEL0} in
  (111) echo "  BOD disabled";;
  (110) echo "  1.8v, min 1.7, max 2.0";;
  (101) echo "  2.7v, min 2.5, max 2.9";;
  (100) echo "  4.3v, min 4.1, max 4.5";;
  (*) echo "UNDEFINED: " ${BODLEVEL2}${BODLEVEL1}${BODLEVEL0}
esac
echo ""

if test $CPUTYPE -eq 48 ; then
  echo "CLOCK"
else
  echo "BOOT AND CLOCK"
  echo "  BOOTSZ:  " ${BOOTSZ1}${BOOTSZ0} "    BOOTRST: " `if test "$BOOTRST" -eq 1 ; then echo "application (0000H)" ; else echo "bootloader" ; fi`
  case ${BOOTSZ1}${BOOTSZ0}${CPUTYPE} in
    (1188)  echo "    128 words (4 pages)  start address:  F80H" ;;
    (11128) echo "    128 words (4 pages)  start address:  1F80H" ;;
    (11328) echo "    256 words (4 pages)  start address:  3F00H" ;;

    (1088)  echo "    256 words (8 pages)  start address:  F00H" ;;
    (10128) echo "    256 words (8 pages)  start address:  1F00H" ;;
    (10328) echo "    512 words (8 pages)  start address:  3E00H" ;;

    (0188)  echo "    512 words (16 pages)  start address:  E00H" ;;
    (01128) echo "    512 words (16 pages)  start address:  1E00H" ;;
    (01328) echo "    1024 words (16 pages)  start address:  3C00H" ;;

    (0088)  echo "    1024 words (32 pages)  start address:  C00H" ;;
    (00128) echo "    1024 words (32 pages)  start address:  1C00H" ;;
    (00328) echo "    2048 words (16 pages)  start address:  3800H" ;;
  esac
fi

echo "  CKSEL:   " ${CKSEL3}${CKSEL2}${CKSEL1}${CKSEL0} "  SUT: " ${SUT1}${SUT0}
case ${CKSEL3}${CKSEL2}${CKSEL1}${CKSEL0} in
  (0000) printf "    External clock" ;
    echo "     " `do_external_clock_normal ${SUT1}${SUT0}` ;;
  (0001) echo "    CKSEL 0001 - Reserved" ;;
  (0010) echo '    Calibrated internal RC oscillator (8Mhz)' ;
    echo "     " `do_external_clock_normal ${SUT1}${SUT0}` ;;
  (0011) echo '    Internal 128khz oscillator' ;
    echo "     " `do_external_clock_normal ${SUT1}${SUT0}` ;;
  (010*)
    echo "    Low Freq Crystal Oscillator";
    case ${CKSEL0}${SUT1}${SUT0} in
      (000) echo  '      fast rising power or BOD enabled, 1k clocks plus 4 clocks';;
      (001) echo  '      slowly rising power, 1k clocks plus 4 clocks plus additional [4.1ms]';;
      (010) echo  '      Stable freq at startup, 1k clocks plus 4 clocks plus additional [65msec]';;
      (011) echo  '      reserved (011)';;
      (100) echo  '      fast rising power or BOD enabled, 32k clocks plus 4 clocks';;
      (101) echo  '      slowly rising power, 32k clocks plus 4 clocks plus additional [4.1ms]';;
      (110) echo  '      Stable freq at startup, 32k clocks plus 4 clocks plus additional [65msec]';;
      (111) echo  '      reserved (111)';;
    esac ;;
  (011*)
    echo "    Full Swing Crystal Oscillator";
    case ${CKSEL0}${SUT1}${SUT0} in
      (000) echo  '      ceramic resonator, fast rising power, 258 clocks plus 14 clocks plus additional [4.1ms]';;
      (001) echo  '      ceramic resonator, slowly rising power, 258 clocks plus 14 clocks plus additional [65ms]';;
      (010) echo  '      ceramic resonator, BOD enabled, 1k clocks plus 14 clocks';;
      (011) echo  '      ceramic resonator, fast rising power, 1k clocks plus 14 clocks plus additional [4.1ms]';;
      (100) echo  '      ceramic resonator, slowly rising power, 1k clocks plus 14 clocks plus additional [65ms]';;
      (101) echo  '      crystal oscillator, BOD enabled, 16k clocks + 14 clocks';;
      (110) echo  '      crystal oscillator, fast rising power, 16k clocks plus 14 clocks plus additional [4.1ms]';;
      (111) echo  '      crystal oscillator, slowly rising power, 16k clocks plus 14 clocks plus additional [65ms]';;
    esac ;;
  (1*)
    printf "    Low Power Crystal Oscillator - ";
    case ${CKSEL2}${CKSEL1} in
      (00) echo 'Low power crystal osc (0.4 to 0.9 Mhz, resonator only)';;
      (01) echo 'Low power crystal osc (0.9 to 3.0 Mhz)';;
      (10) echo 'Low power crystal osc (3.0 to 8.0 Mhz)';;
      (11) echo 'Low power crystal osc (8.0 to 16.0 Mhz)';;
    esac
    case ${CKSEL0}${SUT1}${SUT0} in
      (000) echo  '      ceramic resonator, fast rising power, 258 clocks plus 14 clocks plus additional [4.1ms]';;
      (001) echo  '      ceramic resonator, slowly rising power, 258 clocks plus 14 clocks plus additional [65ms]';;
      (010) echo  '      ceramic resonator, BOD enabled, 1k clocks plus 14 clocks';;
      (011) echo  '      ceramic resonator, fast rising power, 1k clocks plus 14 clocks plus additional [4.1ms]';;
      (100) echo  '      ceramic resonator, slowly rising power, 1k clocks plus 14 clocks plus additional [65ms]';;
      (101) echo  '      crystal oscillator, BOD enabled, 16k clocks + 14 clocks';;
      (110) echo  '      crystal oscillator, fast rising power, 16k clocks plus 14 clocks plus additional [4.1ms]';;
      (111) echo  '      crystal oscillator, slowly rising power, 16k clocks plus 14 clocks plus additional [65ms]';;
    esac ;;
esac

rm /tmp/outX.out
rm /tmp/outE.out
rm /tmp/outH.out
rm /tmp/outL.out


