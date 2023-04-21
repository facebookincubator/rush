# Generate a docker for ffmpeg-rush
# by Jordi Cenzano
# VERSION               1.1

FROM ubuntu:22.04
LABEL maintainer "Jordi Cenzano <jcenzano@meta.com>"

# Update
RUN apt-get update -y

# Upgrade
RUN apt-get upgrade -y

# Install network resources
RUN apt-get -y install iputils-ping net-tools

# Prepare docker for ffmpeg
RUN apt-get -y install curl unzip wget git autoconf automake build-essential cmake git-core libass-dev libfreetype6-dev libgnutls28-dev libmp3lame-dev libsdl2-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev meson ninja-build pkg-config texinfo yasm zlib1g-dev libunistring-dev libaom-dev libdav1d-dev nasm libx264-dev libx265-dev libnuma-dev libvpx-dev libfdk-aac-dev libopus-dev libev-dev

# Install & compile openSSL version for QUIC
RUN cd ~ && \
  git clone --depth 1 -b OpenSSL_1_1_1s+quic https://github.com/quictls/openssl && \
  cd openssl && \
  mkdir build && \
  ./config enable-tls1_3 --prefix=$PWD/build && \
  make -j$(nproc) && \
  make install_sw

# Install & compile nghttp3 v0.8.0
RUN cd ~ && \
  git clone https://github.com/ngtcp2/nghttp3 && \
  cd nghttp3 && \
  mkdir build && \
  git checkout tags/v0.8.0 && \
  autoreconf -i && \
  ./configure --prefix=$PWD/build --enable-lib-only && \
  make -j$(nproc) check && \
  make install

# Install & compile ngtcp2 v0.11.0
RUN cd ~ && \
  git clone https://github.com/ngtcp2/ngtcp2 && \
  cd ngtcp2 && \
  mkdir build && \
  git checkout tags/v0.11.0 && \
  autoreconf -i && \
  ./configure PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../nghttp3/build/lib/pkgconfig LDFLAGS="-Wl,-rpath,$PWD/../openssl/build/lib" --prefix=$PWD/build && \
  make -j$(nproc) check && \
  make install

# Install & compile RUSH
RUN cd ~ && \
  git clone https://github.com/facebookincubator/rush.git && \
  cd ~/rush && \
  mkdir build && \
  export PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../ngtcp2/build/lib/pkgconfig && \
  cmake -DCMAKE_INSTALL_PREFIX=$PWD/build -DBUILD_SHARED_LIBS=OFF -DCMAKE_VERBOSE_MAKEFILE=ON . && \
  make && \
  make install

# Install, patch, and compile ffmpeg 5.0.2
RUN cd ~ && \
  git clone --depth 1 --branch n5.0.2 https://github.com/FFmpeg/FFmpeg.git && \
  cd FFmpeg && \
  git apply $PWD/../rush/ffmpeg-n5.0.2/rush.patch && \
  mkdir ffmpeg_build ffmpeg_bin bin && \
  export PATH="$PWD/bin:$PATH" && \
  export PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../ngtcp2/build/lib/pkgconfig:$PWD/../rush/build/share/pkgconfig && \
  ./configure \
    --prefix="$PWD/ffmpeg_build" \
    --pkg-config-flags="--static" \
    --extra-cflags="-I$PWD/../rush/include" \
    --extra-ldflags="-L$PWD/../rush/build/lib" \
    --extra-libs="-lpthread -lm" \
    --ld="g++" \
    --bindir="$PWD/bin" \
    --enable-gpl \
    --enable-gnutls \
    --enable-libass \
    --enable-libfdk-aac \
    --enable-libfreetype \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-libx264 \
    --enable-libx265 \
    --enable-librush \
    --enable-nonfree \
    --disable-x86asm && \
  make && \
  make install && \
  hash -r

# Tell ffmpeg where to find libs
ENV LD_LIBRARY_PATH /root/openssl/build/lib:/root/ngtcp2/build/lib

# Clean up
RUN apt-get clean

# Start
ENTRYPOINT ["/root/FFmpeg/ffmpeg"]
CMD ["-h"]
