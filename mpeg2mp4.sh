#!/bin/sh
#
# use ffmpeg to convert to mp4 video format from 'whatever it can ingest'
#


if test -z "$1" ; then
  echo Usage:  $0 infile.mpeg outfile.mp3 [options]
  exit
fi

if test -z "$2" ; then
  fname=`echo "$1"  | sed 's/ /\\ /g' | awk '{aa=length($0); a=aa; while(a>0 && substr($0,a,1)!=".") { a--; } print substr($0,1,a); }' | sed 's/\\ / /g'`
  outfile=${fname}mp4
else
  outfile="$2"
fi

ffmpeg -threads 0 -i "${1}" -vcodec libx264 -qscale 0 -ar 22050 -f mp4 "${outfile}"

