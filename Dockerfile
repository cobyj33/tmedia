FROM ubuntu:22.04
RUN apt-get -y update && apt-get install -y
RUN apt-get install -y libncurses5-dev libncursesw5-dev
RUN apt-get update -qq && apt-get -y install \
  autoconf \
  automake \
  build-essential \
  cmake \
  git-core \
  libass-dev \
  libfreetype6-dev \
  libgnutls28-dev \
  libmp3lame-dev \
  libsdl2-dev \
  libtool \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  meson \
  ninja-build \
  pkg-config \
  texinfo \
  wget \
  yasm \
  zlib1g-dev \
  libunistring-dev \
  libaom-dev \
  libdav1d-dev

WORKDIR /

RUN mkdir ffmpeg
RUN mkdir -p /ffmpeg/ffmpeg_sources /ffmpeg/bin

RUN cd /ffmpeg/ffmpeg_sources && \
wget -O ffmpeg-snapshot.tar.bz2 https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2 && \
tar xjvf ffmpeg-snapshot.tar.bz2 && \
cd ffmpeg && \
PATH="/ffmpeg/bin:$PATH" PKG_CONFIG_PATH="/ffmpeg/ffmpeg_build/lib/pkgconfig" ./configure \
  --prefix="/ffmpeg/ffmpeg_build" \
  --pkg-config-flags="--static" \
  --extra-cflags="-I/ffmpeg/ffmpeg_build/include" \
  --extra-ldflags="-L/ffmpeg/ffmpeg_build/lib" \
  --extra-libs="-lpthread -lm" \
  --ld="g++" \
  --bindir="/ffmpeg/bin" \
  --enable-gpl \
  --enable-gnutls \
  --enable-libaom \
  --enable-libass \
#   --enable-libfdk-aac \
  --enable-libfreetype \
  --enable-libmp3lame \
#   --enable-libopus \
#   --enable-libsvtav1 \
  --enable-libdav1d \
  --enable-libvorbis \
#   --enable-libvpx \
#   --enable-libx264 \
#   --enable-libx265 \
  --enable-nonfree && \
PATH="/ffmpeg/bin:$PATH" make -j4 && \
make install && \
hash -r

# RUN source ~/.profile
# RUN apt-get autoremove -y autoconf automake build-essential cmake git-core libass-dev libfreetype6-dev libgnutls28-dev libmp3lame-dev libnuma-dev libopus-dev libsdl2-dev libtool libva-dev libvdpau-dev libvorbis-dev libvpx-dev libx264-dev libx265-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev texinfo wget yasm zlib1g-dev
RUN apt-get autoremove -y
ENV NAME TYPE
ENV NAME LOCATION

RUN mkdir ascii_video
WORKDIR /ascii_video
COPY src src
COPY includes includes
COPY CMakeLists.txt CMakeLists.txt
COPY assets assets
RUN export PKG_CONFIG_PATH="/ffmpeg/ffmpeg_build/lib/pkgconfig:$PKG_CONFIG_PATH" && mkdir build &&  cd build && cmake ../ && make -j4


ENTRYPOINT ["/ascii_video/build/ascii_video"]

# Ubuntu Command: sudo docker run --device /dev/snd --rm -v <video> -it ascii_video -v /video.mp4
# Change <video> to a video path on your machine