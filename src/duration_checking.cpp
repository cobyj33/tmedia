#include "mediafetcher.h"

#include "sleep.h"
#include "wtime.h"

#include <mutex>

void MediaFetcher::duration_checking_thread_func() {
  if (this->media_type == MediaType::IMAGE) return;
  const int DURATION_CHECKING_LOOP_TIME_MS = 100;

  while (this->in_use) {
    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->get_time(system_clock_sec()) >= this->get_duration()) {
        this->in_use = false;
        break;
      }
    }

    sleep_for_ms(DURATION_CHECKING_LOOP_TIME_MS);
  }
}