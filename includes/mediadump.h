#ifndef ASCII_VIDEO_DUMP
#define ASCII_VIDEO_DUMP

#include <media.h>

int get_media_player_dump_size(MediaPlayer* player);
void dump_media_player(MediaPlayer* player, char* buffer);

int get_media_timeline_dump_size(MediaTimeline* timeline);
void dump_media_timeline(MediaTimeline* timeline, char* buffer);

int get_media_data_dump_size(MediaData* data);
void dump_media_data(MediaData* data, char* buffer);

int get_media_stream_dump_size(MediaStream* stream);
void dump_media_stream(MediaStream* stream, char* buffer);

int get_stream_info_dump_size(StreamData* stream_data);
void dump_stream_info(StreamData* stream_data, char* buffer);

int get_playback_dump_size(Playback* playback);
void dump_playback(Playback* playback, char* buffer);
#endif
