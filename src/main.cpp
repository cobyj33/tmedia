#include "signalstate.h"

#include "wtime.h"
#include "avguard.h"
#include "tmcurses.h"
#include "tmedia.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <csignal>

extern "C" {
#include <libavutil/log.h>
}


bool INTERRUPT_RECEIVED = false; //defined as extern in signalstate.h
void interrupt_handler(int) {
  INTERRUPT_RECEIVED = true;
}

void on_terminate() {
  ncurses_uninit();

  std::exception_ptr exptr = std::current_exception();
  try {
    std::rethrow_exception(exptr);
  }
  catch (std::exception const& ex) {
    std::cerr << "[tmedia] Terminated due to exception: \n\t" <<
      ex.what() << std::endl;
  }

  std::abort();
}


int main(int argc, char** argv)
{
  #if USE_AV_REGISTER_ALL
  av_register_all();
  #endif
  #if USE_AVCODEC_REGISTER_ALL
  avcodec_register_all();
  #endif

  std::signal(SIGINT, interrupt_handler);
	std::signal(SIGTERM, interrupt_handler);
	std::signal(SIGABRT, interrupt_handler);
  
  srand(time(nullptr));
  av_log_set_level(AV_LOG_QUIET);
  std::set_terminate(on_terminate);

  TMediaCLIParseRes res = tmedia_parse_cli(argc, argv);
  if (res.exited)
    return EXIT_SUCCESS;
  return tmedia_run(res.tmss);
}