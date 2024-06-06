# media

Some relevant sections in the FFmpeg documentation where more can be learned
about decoding video and audio are:

- [libavformat module](https://ffmpeg.org/doxygen/trunk/group__libavf.html)
- [send/receive encoding and decoding API overview](https://ffmpeg.org/doxygen/trunk/group__lavc__encdec.html)
- [libavcodec decoding module](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html)
- [libavcodec core structs](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html)
- [libavformat demuxing docs](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html)

The structure of the media module forms somewhat of a complexity stack up
until the MediaFetcher interface. From simply reading and decoding packets
to coordinating that reading with a current playing time, each part is made
to take on a responsibility along the stack of multimedia concerns.

## MediaFetcher

  The MediaFetcher is the main backbone of tmedia. The MediaFetcher is
  responsible for coordinating the decoding of media data
  with an actual running duration. The MediaFetcher runs different threads for
  decoding video and audio, and has fields which hold current audio data and
  video data lining up with the current part of media playing. Additionally,
  methods such as begin(), join(), pause(), and resume() help coordinate the
  MediaFetcher with the higher level media player which lives in tmedia.cpp.
  The MediaFetcher however is NOT responsible for audio output or video output,
  only getting the data needed for them.

## MediaDecoder

  The MediaDecoder is analogous to the AVFormatContext struct, but is wrapped
  to specifically and easily handle fetching data frames from a media file
  linearly (MediaDecoder::decode_next) and to seek to a specific time in the
  media file upon request (MediaDecoder::jump_to_time). The MediaDecoder is also
  responsible for fetching the packets for its individual StreamDecoders, and
  for exposing general media metadata from its AVFormatContext and its media
  streams. 

## StreamDecoder

  The stream decoder is not responsible for fetching its own packets, as in the
  FFmpeg API the AVFormatContext holds all of the relevant information for parsing
  and demuxing the media stream. Instead, the stream decoder holds a queue of
  demuxed packets and decodes them upon request through
  StreamDecoder::decode_next(), utilizing the decode_packet_queue function.

## decode_packet_queue

  Many times, decoding a video or audio packet can fail with AVERROR(EAGAIN),
  which signifies that more media data is required in the form of AVPackets to
  decode actual AVFrames. Because of this tendency, it helps to have a 
  function which chews through a deque of packets such as decode_packet_queue,
  which takes packets from the given deque until valid AVFrame's are encountered
  to return, or an unrecoverable FFmpeg error has occured.

## decode_video_packet and decode_audio_packet

  These functions abstract the steps to decode a video and audio packet
  respectively into a stream of AVFrame's, which contain the frame data from
  the media file being currently read. Note that since one packet can return
  multiple AVFrames, each packet can return an std::vector of AVFrame pointers.
  These functions "return" their AVFrames through a referenced buffer passed in
  by the caller, which must have a length of 0, so that the caller can
  repeatedly fill AVFrames in a buffer without reallocating
  the buffer each call.

More specific information can be found on each layer in their respective
.h and .cpp files.