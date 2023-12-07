#include "signalstate.h"

#include "wtime.h"
#include "mediafetcher.h"
#include "formatting.h"
#include "version.h"
#include "avguard.h"
#include "tmcurses.h"
#include "playlist.h"
#include "tmcurses.h"
#include "ascii.h"
#include "wminiaudio.h"
#include "sleep.h"
#include "wmath.h"
#include "boiler.h"
#include "miscutil.h"
#include "tmedia.h"
#include "tmedia_vom.h"

#include <cstdlib>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <csignal>
#include <map>
#include <algorithm>

extern "C" {
#include "curses.h"
#include "miniaudio.h"
#include <libavutil/log.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libavcodec/version.h>
#include <libswresample/version.h>
#include <libswscale/version.h>
}

int program_print_ffmpeg_version();
int program_print_curses_version();
int program_dump_metadata(const std::vector<std::string>& files);
std::vector<std::string> resolve_cli_paths(const std::vector<std::string>& paths, bool recursive);

bool INTERRUPT_RECEIVED = false; //defined as extern in signalstate.h
void interrupt_handler(int) {
  INTERRUPT_RECEIVED = true;
}

void on_terminate() {
  if (ncurses_is_initialized()) {
    ncurses_uninit();
  }

  std::exception_ptr exptr = std::current_exception();
  try {
    std::rethrow_exception(exptr);
  }
  catch (std::exception const& ex) {
    std::cerr << "[tmedia] Terminated due to exception: " << ex.what() << std::endl;
  }

  std::abort();
}

template <typename T>
std::string format_arg_map(std::map<std::string, T> map, std::string conjunction) {
  return format_list(transform_vec(get_strmap_keys(map), [](std::string str) { return "'" + str + "'"; }), conjunction);
}


int main(int argc, char** argv)
{
  #if USE_AV_REGISTER_ALL
  av_register_all();
  #endif
  #if USE_AVCODEC_REGISTER_ALL
  avcodec_register_all();
  #endif
  
  srand(time(nullptr));
  av_log_set_level(AV_LOG_QUIET);
  std::set_terminate(on_terminate);

  TMediaCLIParseRes res = tmedia_parse_cli(argc, argv);
  if (res.exited)
    return EXIT_SUCCESS;
  return tmedia_run(res.tmss);
}