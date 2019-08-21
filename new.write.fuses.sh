#!/bin/sh

if test -z "$4" ; then
  echo usage:  new.write.fuses.sh CPU L H eflags
  echo lock bits are always set to ffH
  exit
fi

CPU=$1
LFLAGS=$2
HFLAGS=$3
EFLAGS=$4

/usr/local/bin/avrdude -cavrispmkII -Pusb -v -C/usr/local/etc/avrdude.conf \
  -p${CPU} \
  -Ulock:w:0xff:m -Uefuse:w:$EFLAGS:m -Uhfuse:w:$HFLAGS:m -Ulfuse:w:$LFLAGS:m

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

