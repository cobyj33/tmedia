#include "sigexit.h"

#include <atomic>

std::atomic<bool> sent_exit_signal;

bool should_sig_exit() {
  return sent_exit_signal;
}

void ascii_video_signal_handler(int signal) {
  sent_exit_signal = true;
}