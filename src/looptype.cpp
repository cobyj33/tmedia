#include "looptype.h"

#include <stdexcept>

int playback_get_next(int current, int max, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == max ? -1 : current + 1;
    case LoopType::REPEAT: return current + 1 == max ? 0 : current + 1;
    case LoopType::REPEAT_ONE: return current;
  }
  throw std::runtime_error("[playback_get_next] Could not identify unknown loop_type");
}

int playback_get_previous(int current, int max, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current - 1 == 0 ? 0 : current + 1;
    case LoopType::REPEAT: return current - 1 < 0 ? max - 1 : current - 1;
    case LoopType::REPEAT_ONE: return current;
  }
  throw std::runtime_error("[playback_get_previous] Could not identify unknown loop_type");
}

std::string loop_type_to_string(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "no loop";
    case LoopType::REPEAT: return "repeat";
    case LoopType::REPEAT_ONE: return "repeat one";
  }
  throw std::runtime_error("[loop_type_to_string] Could not identify unknown loop_type");
}