FROM ubuntu:22.04

RUN apt-get update 
RUN apt-get install -y build-essential cmake pkg-config git libtool

RUN mkdir ascii_video
WORKDIR /ascii_video
COPY src src
COPY includes includes
COPY lib lib
RUN rm -rf lib/bin
COPY assets assets
COPY CMakeLists.txt CMakeLists.txt

RUN mkdir build
WORKDIR /ascii_video/build
RUN cmake ../
RUN make -j2

ENTRYPOINT ["/ascii_video/build/ascii_video"]

# Ubuntu Command: docker run --rm -it --device=/dev/snd -v=<PATH TO VIDEO>:/video.mp4 ascii_video -v /video.mp4
# Note that the above command may need "sudo" prefixed in front
# to run on debian systems, or some other alternative
# Change <PATH TO VIDEO> to a video path on your machine