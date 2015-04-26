Download ffmpeg from https://github.com/nastasi/FFmpeg.git in:
  <basedir>/git/ffmpeg

Compile and install ffmpeg with:
./configure --prefix=$HOME/ffmpeg --disable-static --enable-shared --enable-pic --enable-dynamic-filter
make
make install

Download vf_chronograph in:
  <basedir>/git/vf_chronograph

Compile and install:
make
./test.sh

take a look to the "out" directory, and try to play the generated mpg video.
