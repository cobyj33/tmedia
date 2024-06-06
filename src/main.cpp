#include <tmedia/signalstate.h>

#include <tmedia/util/wtime.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/tmedia.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <clocale>

extern "C" {
#include <libavutil/log.h>
#include <unistd.h>
}


bool INTERRUPT_RECEIVED = false; //defined as extern in tmedia/signalstate.h
void interrupt_handler(int) {
  INTERRUPT_RECEIVED = true;
}

void on_terminate() {
  tmcurses_uninit();

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


int main(int argc, char** argv) {
  if (!isatty(STDIN_FILENO)) {
    std::cerr << "[tmedia]: stdin must be a tty. Exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  setlocale(LC_ALL, "");

  #if USE_AV_REGISTER_ALL
  av_register_all();
  #endif
  #if USE_AVCODEC_REGISTER_ALL
  avcodec_register_all();
  #endif

  std::signal(SIGINT, interrupt_handler);
	std::signal(SIGTERM, interrupt_handler);

  /*
  Otherwise, it would be difficult to justify using C's assert macro in code to
  handle logical errors, as we could not guarantee that every abort would
  exit the program and every abort would uninitialize curses beforehand, leading
  to returning the calling terminal into its normal state.
  Source: (https://www.man7.org/linux/man-pages/man3/abort.3.html)
  */
	std::signal(SIGABRT, interrupt_handler);

  // I honestly forgot why I did this, but I'll just leave it as is for now.
  // (no, srand() is not called to randomize the seed for shuffling the
  // playlist, as https://github.com/ilqvya/random is used for that
  // funcitonality)
  srand(time(nullptr));
  av_log_set_level(AV_LOG_QUIET);
  std::set_terminate(on_terminate);

  TMediaCLIParseRes res = tmedia_parse_cli(argc, argv);
  if (res.exited) {
    return EXIT_SUCCESS;
  }
  
  if (res.errors.size() > 0) {
    std::cerr << "tmedia: Errors while parsing CLI arguments:" << std::endl;
    for (const std::string& err : res.errors) {
      std::cerr << "  " <<  err << std::endl;
    }
    return EXIT_FAILURE;
  }

  return tmedia_run(res.tmss);
}
