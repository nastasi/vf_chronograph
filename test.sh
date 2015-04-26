#!/bin/bash
set -x
export FFMPEG_DYN_FILTERS="${PWD}/plugin"
VIDEO_OUT=out/video.mpg
export LD_BIND_NOW=y
export LD_LIBRARY_PATH=/home/nastasi/ffmpeg/lib:
export GDFONTPATH=/usr/share/fonts/truetype
# /ubuntu-font-family/UbuntuMono-R.ttf

if [ ! -d out ]; then
    mkdir out
fi
if [ -f out/video.mpg ]; then
    rm out/video.mpg
fi
CDUMP=
if [ $CDUMP ]; then
   GDB=gdb
   GDB_ARGS=--args
fi
rm -f $VIDEO_OUT
$GDB $GDB_ARGS ffmpeg -loglevel info -i media/video_in.mp4 -r 59.94 -vb 25M -vf "chronograph=1400:200:200" $VIDEO_OUT

