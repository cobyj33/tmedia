#include <tmedia/media/mediafetcher.h>

#include <tmedia/util/sleep.h>
#include <tmedia/util/wtime.h>

#include <mutex>
using ms = std::chrono::milliseconds;

/**
 * Simple MediaFetcher thread that constantly polls the media clock to check
 * if the clock has surpassed the loaded media's duration.
*/
void MediaFetcher::duration_checking_thread_func() {
  if (this->media_type == MediaType::IMAGE) return;
  constexpr ms DC_LOOP_TIME_MS = ms(100);

  while (!this->should_exit()) {
    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->get_time(sys_clk_sec()) >= this->get_duration()) {
        this->dispatch_exit();
        break;
      }
    }

    std::unique_lock<std::mutex> exit_lock(this->ex_noti_mtx);
    if (!this->should_exit()) {
      this->exit_cond.wait_for(exit_lock, DC_LOOP_TIME_MS);
    }
  }
}