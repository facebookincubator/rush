# docker-ffmpeg-rush
Docker image definition with compiled ffmpeg and RUSH library that allows you to stream to Meta using RUSH protocol

# Pulling docker image from docker hub
1. Ensure you have [docker](https://www.docker.com) installed
2. Type: `docker pull facebookincubator/docker-ffmpeg-rush`

# Creating the docker image locally (optional)
1. Ensure you have docker [docker](https://www.docker.com) and make installed
2. Type `make`

# Testing the image
You can test the image with this command (you should see ffmpeg help):
```
docker run --rm -it facebookincubator/docker-ffmpeg-rush
```

# Using the image
You can call ffmpeg-rush docker like you call the local command, just adding the ffmpeg parameters at the end of the docker run call, example:
```
docker run --rm -it facebookincubator/docker-ffmpeg-rush -hide_banner -y -fflags +genpts -f lavfi -i smptebars=duration=300:size=640x360:rate=30 -re -f lavfi -i sine=duration=300:frequency=1000:sample_rate=44100 -c:v libx264 -preset medium -profile:v baseline -g 60 -b:v 1000k -maxrate:v 1200k -bufsize:v 2000k -a53cc 0 -c:a aac -b:a 128k -ac 2 -vf "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf: text=\'Local time %{localtime\: %Y\/%m\/%d %H.%M.%S} (%{n})\': x=10: y=10: fontsize=16: fontcolor=white: box=1: boxcolor=0x00000099" -f rush "rush://live-api-s.facebook.com:443/rtmp/STREAM_KEY"
```
Another example:
```
docker run --rm -it facebookincubator/docker-ffmpeg-rush -i test-input.flv -f rush "rush:://serverURL/streamKey"
```
