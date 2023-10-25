#ifndef ASCII_VIDEO_LOOP_TYPE_H
#define ASCII_VIDEO_LOOP_TYPE_H

#include <string>

enum class LoopType {
  REPEAT_ONE,
  REPEAT,
  NO_LOOP
};

int playback_get_next(int current, int max, LoopType loop_type);
int playback_get_previous(int current, int max, LoopType loop_type);
std::string loop_type_to_string(LoopType loop_type);

#endif