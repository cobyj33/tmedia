# media


As a little prerequisite, I'll dedicate a small section here to somewhat
explain the Media Decoding API from FFmpeg 

Some relevant sections in the FFmpeg documentation where more can be learned
are:

- [send/receive encoding and decoding API overview](https://ffmpeg.org/doxygen/trunk/group__lavc__encdec.html)
- [libavcodec decoding module](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html)
- [libavcodec core structs](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html)
- [libavformat module](https://ffmpeg.org/doxygen/trunk/group__libavf.html)
- [libavformat demuxing docs](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html)


The structure of the media module forms somewhat of a complexity stack up
until the MediaFetcher interface. From simply reading and decoding packets
to 

- MediaFetcher

- MediaDecoder
  The MediaDecoder is analogous to the AVFormatContext struct, but is wrapped
  to specifically and easily handle fetching data frames from a media file
  linearly (MediaDecoder::decode_next) and to seek to a specific time in the
  media file upon request

- StreamDecoder
  The stream decoder is not responsible for fetching its own packets, as in the
  FFmpeg API the AVFormatContext holds all of the relevant information for parsing
  and demuxing the media stream. Instead, the stream decoder holds a queue of
  demuxed packets and decodes them upon request through
  StreamDecoder::decode_next();

- decode_video_packet and decode_audio_packet
  These functions abstract the steps to decode a video and audio packet
  respectively into a stream of AVFrame's which contain the frame data from
  the media file being currently read.


More specific information can be found on each layer in their respective
cpp files.