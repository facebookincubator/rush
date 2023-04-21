# Introduction
This library will enable you to send a media stream using RUSH protocol to any server accepting it (you can use FBlive for instance)

## What is RUSH
RUSH (Reliable - unreliable - Streaming Protocol) is a bidirectional application-level protocol designed for live video ingestion that runs on top of QUIC.
RUSH was built as a replacement for RTMP (Real-Time Messaging Protocol) with the goal to provide support for new audio and video codecs, extensibility in the form of new message types, and multi-track support. In addition, RUSH gives applications the option to control data delivery guarantees by utilizing QUIC streams.
See [RUSH draft](https://www.ietf.org/archive/id/draft-kpugin-rush-00.html).

# I want to use it
There different ways to use this library

## Docker
If you just want pull a pre built docker image, you just need to:

1. Make sure you have Docker installed in your machine. See [get docker](https://docs.docker.com/get-docker/)
2. Pull the docker image and start streaming by doing:
```
docker run --rm -it facebookincubator/docker-ffmpeg-rush:1.1 -hide_banner -y -fflags +genpts -f lavfi -i smptebars=duration=300:size=640x360:rate=30 -re -f lavfi -i sine=duration=300:frequency=1000:sample_rate=44100 -c:v libx264 -preset medium -profile:v baseline -g 60 -b:v 1000k -maxrate:v 1200k -bufsize:v 2000k -a53cc 0 -c:a aac -b:a 128k -ac 2 -vf "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf: text=\'Local time %{localtime\: %Y\/%m\/%d %H.%M.%S} (%{n})\': x=10: y=10: fontsize=16: fontcolor=white: box=1: boxcolor=0x00000099" -f rush RUSH_URL
```
3. OR Create a RUSH file by doing:
```
docker run --rm -it facebookincubator/docker-ffmpeg-rush:1.1 -hide_banner -y -fflags +genpts -f lavfi -i smptebars=duration=300:size=640x360:rate=30 -re -f lavfi -i sine=duration=300:frequency=1000:sample_rate=44100 -c:v libx264 -preset medium -profile:v baseline -g 60 -b:v 1000k -maxrate:v 1200k -bufsize:v 2000k -a53cc 0 -c:a aac -b:a 128k -ac 2 -vf "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf: text=\'Local time %{localtime\: %Y\/%m\/%d %H.%M.%S} (%{n})\': x=10: y=10: fontsize=16: fontcolor=white: box=1: boxcolor=0x00000099" -f rush MY_FILE.rush
```

## Building this library
You should follow the next steps to build the library in a Ubuntu AWS instance

1. Spin up AWS ubuntu machine

2. Create new SSH key and add it to your user [instructions](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent)

3. SSH that AWS machine

4. Clone public [ffmpeg repo](https://ffmpeg.org/download.html)
```
cd ~
git clone --depth 1 --branch n5.0.2 https://github.com/FFmpeg/FFmpeg.git
```

5. Clone RUSH code (this lib)
```
cd ~
git clone git@github.com:facebookincubator/rush.git
```

6. Apply RUSH patch to ffmpeg
```
cd ~
cd FFmpeg
git apply ../rush/ffmpeg-n5.0.2/rush.patch
```

7. Install some dependencies (basically ffmpeg ones)
```
cd ~
cd FFmpeg
sudo apt-get update -qq && sudo apt-get -y install \
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
  libdav1d-dev \
  nasm \
  libx264-dev \
  libx265-dev \
  libnuma-dev \
  libvpx-dev \
  libfdk-aac-dev \
  libopus-dev \
  libev-dev
  ```

8. Install & compile openssl version for QUIC, and specific versions of nghttp3, and Ngtcp2
```
cd ~
git clone --depth 1 -b OpenSSL_1_1_1s+quic https://github.com/quictls/openssl
cd openssl
mkdir build
./config enable-tls1_3 --prefix=$PWD/build
make -j$(nproc)
make install_sw
cd ..
git clone https://github.com/ngtcp2/nghttp3
cd nghttp3
mkdir build
git checkout tags/v0.8.0
autoreconf -i
./configure --prefix=$PWD/build --enable-lib-only
make -j$(nproc) check
make install
cd ..
git clone https://github.com/ngtcp2/ngtcp2
cd ngtcp2
mkdir build
git checkout tags/v0.11.0
autoreconf -i
./configure PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../nghttp3/build/lib/pkgconfig LDFLAGS="-Wl,-rpath,$PWD/../openssl/build/lib" --prefix=$PWD/build
make -j$(nproc) check
make install
```

9. Compile RUSH code
```
cd ~
cd rush
mkdir build
export PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../ngtcp2/build/lib/pkgconfig
cmake -DCMAKE_INSTALL_PREFIX=$PWD/build -DBUILD_SHARED_LIBS=OFF -DCMAKE_VERBOSE_MAKEFILE=ON .
make
make install
```

10. Compile ffmpeg with RUSH
```
cd ~
cd FFmpeg
mkdir ffmpeg_build ffmpeg_bin bin
export PATH="$PWD/bin:$PATH"
export PKG_CONFIG_PATH=$PWD/../openssl/build/lib/pkgconfig:$PWD/../ngtcp2/build/lib/pkgconfig:$PWD/../rush/build/share/pkgconfig ./configure \
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
  --disable-x86asm
make
make install
hash -r
```

Start streaming by doing:
```
cd ~
cd FFmpeg
export LD_LIBRARY_PATH=$PWD/../openssl/build/lib:$PWD/../ngtcp2/build/lib

./ffmpeg -hide_banner -y -fflags +genpts -f lavfi -i smptebars=duration=300:size=640x360:rate=30 -re -f lavfi -i sine=duration=300:frequency=1000:sample_rate=44100 -c:v libx264 -preset medium -profile:v baseline -g 60 -b:v 1000k -maxrate:v 1200k -bufsize:v 2000k -a53cc 0 -c:a aac -b:a 128k -ac 2 -vf "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf: text=\'Local time %{localtime\: %Y\/%m\/%d %H.%M.%S} (%{n})\': x=10: y=10: fontsize=16: fontcolor=white: box=1: boxcolor=0x00000099" -f rush RUSH_URL
```

# Project Roadmap
The project current is under active development and the future roadmap includes:
 - Server-side RUSH implementation
 - Multi-stream mode
 - Integration with popular streaming projects
 - Support for all Operating Systems

# License
RUSH is release under the [MIT License](https://github.com/facebookincubator/rush/blob/master/LICENSE).
