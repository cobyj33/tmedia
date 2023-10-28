#include "mediafetcher.h"

#include "sleep.h"
#include "wtime.h"

#include <mutex>

void MediaFetcher::duration_checking_thread_func() {
  if (this->media_type == MediaType::IMAGE) return;
  const int DURATION_CHECKING_LOOP_TIME_MS = 100;

  while (!this->should_exit()) {
    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->get_time(system_clock_sec()) >= this->get_duration()) {
        this->dispatch_exit();
        break;
      }
    }

    std::unique_lock<std::mutex> exit_lock(this->exit_notify_mutex);
    if (!this->should_exit()) {
      this->exit_cond.wait_for(exit_lock, std::chrono::milliseconds(DURATION_CHECKING_LOOP_TIME_MS));
    }
  }
}