#!/bin/sh

# printing a banner for a program based on entered text

THE_YEAR=`date | awk '{ print $6 }'`
STRING1="Copyright (c) "$THE_YEAR" by Your Organization"
STRING2="Use, copying, and distribution of this software are licensed according"
STRING3="to the GPLv2, LGPLv2, or BSD license, as appropriate (see COPYING)"

STRING1_LEN=`echo $STRING1 | awk '{ print length($0); }'`
STRING2_LEN=`echo $STRING2 | awk '{ print length($0); }'`
STRING3_LEN=`echo $STRING3 | awk '{ print length($0); }'`

max()
{
  xx=$1
  shift
  yy=$1
  while test -n "$yy" ; do
    if test $xx -lt $yy ; then xx=$yy ; fi
    shift
    yy=$1
  done
  echo $xx
  return 0
}

center()
{
  WIDTH=$1
  shift
  echo '{ W=' $WIDTH \
    '; L=length($0); Y=""; for(C=0; C < (W - L) / 2; C++) { Y=Y " "; } Y=Y $0; for(C=length(Y); C < W; C++) { Y=Y " "; } print "//" Y "//"; }' \
    >/var/tmp/bannertmp2.txt

  awk -f /var/tmp/bannertmp2.txt
  rm /var/tmp/bannertmp2.txt
  return 0
}

divider()
{
  WIDTH=$1

  echo '{ W=' $WIDTH '; L=length($0); Y=""; for(C=0; C < W; C++) { Y=Y "/"; } print "//" Y "//"; }' \
    >/var/tmp/bannertmp2.txt

  echo "" | awk -f /var/tmp/bannertmp2.txt
  return 0
}

# uses the 'figlet' program to do all of the REAL work
figlet -k -w 999 $@ >/var/tmp/bannertmp.txt

LINE_LEN=`head -n 1 /var/tmp/bannertmp.txt | awk '{ print length($0); }'`
MAX_LEN=`max $LINE_LEN $STRING1_LEN $STRING2_LEN $STRING3_LEN`
MAX_LEN2=`expr $MAX_LEN + 4`

echo `divider $MAX_LEN2`
echo "" | center $MAX_LEN2
cat /var/tmp/bannertmp.txt | center $MAX_LEN2
echo "" | center $MAX_LEN2
echo `divider $MAX_LEN2`
echo "" | center $MAX_LEN2
echo $STRING1 | center $MAX_LEN2
echo $STRING2 | center $MAX_LEN2
echo $STRING3 | center $MAX_LEN2
echo "" | center $MAX_LEN2
echo `divider $MAX_LEN2`
rm /var/tmp/bannertmp.txt

