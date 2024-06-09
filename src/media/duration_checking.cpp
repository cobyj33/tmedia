#include <tmedia/media/mediafetcher.h>

#include <tmedia/util/thread.h>
#include <tmedia/util/wtime.h>

#include <mutex>
using namespace std::chrono_literals;

/**
 * Simple MediaFetcher thread that constantly polls the media clock to check
 * if the clock has surpassed the loaded media's duration.
*/
void MediaFetcher::duration_checking_thread_func() {
  if (this->media_type == MediaType::IMAGE) return;
  constexpr std::chrono::milliseconds DC_LOOP_TIME_MS = 100ms;
  name_current_thread("tmedia_durcheck");

  while (!this->should_exit()) {
    // since this thread is so simple, don't even bother with checking if
    // paused or not

    {
      std::lock_guard<std::mutex> alter_lock(this->alter_mutex);
      if (this->get_time(sys_clk_sec()) >= this->duration) {
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