#!/bin/sh
#
# use ffmpeg and twolame to extract sound from a video file (originally
# was for flash video, now mp4 or avi or webm or whatever ffmpeg can
# use as input).  Converts to mp3 using twolame.
#

if test -z "${1}${2}" ; then
  echo Usage:  $0 infile.flv outfile.mp3 [options]
  echo "        also can be used for mp4, avi, webm, etc."
  exit
fi

rm -f .tempfile.wav

ffmpeg -i "$1" -map 0:a -c:a:0 pcm_s16le ".tempfile.wav"

twolame ".tempfile.wav" "$2"

rm -f .tempfile.wav


