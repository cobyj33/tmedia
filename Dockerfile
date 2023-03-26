FROM ubuntu:22.04

RUN apt-get update 
RUN apt-get install -y build-essential cmake pkg-config

RUN mkdir ascii_video
WORKDIR /ascii_video
COPY src src
COPY includes includes
COPY lib lib
RUN rm -rf lib/bin
COPY assets assets
COPY CMakeLists.txt CMakeLists.txt

COPY install.sh install.sh
RUN chmod +x ./install.sh
RUN ./install.sh
RUN apt-get autoremove -y

ENTRYPOINT ["/ascii_video/build/ascii_video"]

# Ubuntu Command: sudo docker run --device /dev/snd --rm -v <video>:/video -it ascii_video /video
# Change <video> to a video path on your machine