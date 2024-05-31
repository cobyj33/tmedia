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


#undef NEWDELTRACKER_AT_OR_ABOVE_CXX17
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
# define NEWDELTRACKER_AT_OR_ABOVE_CXX17 1
#endif

namespace newdelwatch {
  std::size_t new_call_count = 0;
  std::size_t new_array_call_count = 0;

  std::size_t delete_call_count = 0;
  std::size_t delete_sized_call_count = 0;
  std::size_t delete_array_call_count = 0;
  std::size_t delete_sized_array_call_count = 0;

  #if defined(NEWDELTRACKER_AT_OR_ABOVE_CXX17)
  std::size_t new_align_call_count = 0;
  std::size_t new_array_align_call_count = 0;

  std::size_t delete_align_call_count = 0;
  std::size_t delete_sized_align_call_count = 0;
  std::size_t delete_array_align_call_count = 0;
  std::size_t delete_sized_array_align_call_count = 0;
  #endif

  void reset();
  void report(std::ostream& ostream);

  void reset() {
    new_call_count = 0;
    new_array_call_count = 0;

    delete_call_count = 0;
    delete_sized_call_count = 0;
    delete_array_call_count = 0;
    delete_sized_array_call_count = 0;

    #if defined(NEWDELTRACKER_AT_OR_ABOVE_CXX17) || _MSVC_LANG >= 201703L
    new_align_call_count = 0;
    new_array_align_call_count = 0;

    delete_align_call_count = 0;
    delete_sized_align_call_count = 0;
    delete_array_align_call_count = 0;
    delete_sized_array_align_call_count = 0;
    #endif
  }

  /**
   * Utility function to print all allocation trackers to stdout
   * @param ostream The output stream to print character data to for printing
  */
  void report(std::ostream& ostream) {
    ostream << "\t" << new_call_count << " calls to new tracked" << std::endl;
    ostream << "\t" << new_array_call_count << " calls to new[] tracked" << std::endl;

    ostream << "\t" << delete_call_count << " calls to delete(void*) tracked" << std::endl;
    ostream << "\t" << delete_sized_call_count << " calls to delete(void*, std::size_t) tracked" << std::endl;
    ostream << "\t" << delete_array_call_count << " calls to delete[](void*) tracked" << std::endl;
    ostream << "\t" << delete_sized_array_call_count << " calls to delete[](void*, std::size_t) tracked" << std::endl;


    #if defined(NEWDELTRACKER_AT_OR_ABOVE_CXX17)
    ostream << "\t" << new_align_call_count << " calls to aligned new tracked" << std::endl;
    ostream << "\t" << new_array_align_call_count << " calls to aligned new[] tracked" << std::endl;

    ostream << "\t" << delete_align_call_count << " calls to aligned delete(void*) tracked" << std::endl;
    ostream << "\t" << delete_sized_align_call_count << " calls to aligned delete(void*, std::size_t) tracked" << std::endl;
    ostream << "\t" << delete_array_align_call_count << " calls to aligned delete[](void*) tracked" << std::endl;
    ostream << "\t" << delete_sized_array_align_call_count << " calls to aligned delete[](void*, std::size_t) tracked" << std::endl;
    #endif
  }
}

void* generic_new(const std::size_t sz) {
  void *ptr = std::malloc(sz);
  if (ptr) {
    return ptr;
  } else {
    throw std::bad_alloc{};
  }
}

// Unaligned New and Delete New Overrides

void* operator new(const std::size_t sz) {
  newdelwatch::new_call_count++;
	return generic_new(sz);
}

void* operator new[](const std::size_t sz) {
  newdelwatch::new_array_call_count++;
	return generic_new(sz);
}

// Delete Overrides

void operator delete(void * ptr) noexcept(true) {
  newdelwatch::delete_call_count++;
  std::free(ptr);
}

void operator delete(void * ptr, std::size_t sz) noexcept(true) {
  newdelwatch::delete_sized_call_count++;
  std::free(ptr);
  (void)sz;
}

void operator delete[](void* ptr) noexcept{
  newdelwatch::delete_array_call_count++;
	std::free(ptr);
}

void operator delete[](void* ptr, std::size_t sz) noexcept{
  newdelwatch::delete_sized_array_call_count++;
	std::free(ptr);
  (void)sz;
}


#if defined(NEWDELTRACKER_AT_OR_ABOVE_CXX17)
// Aligned new and delete (C++17+)
// (https://en.cppreference.com/w/cpp/memory/new/operator_new)
// (https://en.cppreference.com/w/cpp/memory/new/operator_delete)

// This'll probably break something because its not actually
// aligning anything in memory... but whatever we'll see
void* operator new(const std::size_t sz, std::align_val_t al) {
  newdelwatch::new_align_call_count++;
	return generic_new(sz);
  (void)al;
}

void* operator new[](const std::size_t sz, std::align_val_t al) {
  newdelwatch::new_array_align_call_count++;
	return generic_new(sz);
  (void)al;
}

void operator delete(void * ptr, std::align_val_t al) noexcept(true) {
  newdelwatch::delete_align_call_count++;
  std::free(ptr);
  (void)al;
}

void operator delete(void * ptr, std::size_t sz, std::align_val_t al) noexcept(true) {
  newdelwatch::delete_sized_align_call_count++;
  std::free(ptr);
  (void)sz;
  (void)al;
}

void operator delete[](void* ptr, std::align_val_t al) noexcept{
  newdelwatch::delete_array_align_call_count++;
	std::free(ptr);
  (void)al;
}

void operator delete[](void* ptr, std::size_t sz, std::align_val_t al) noexcept{
  newdelwatch::delete_sized_array_align_call_count++;
	std::free(ptr);
  (void)sz;
  (void)al;
}
#endif


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
	std::signal(SIGABRT, interrupt_handler);

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

  int exitcode = tmedia_run(res.tmss);
  newdelwatch::report(std::cout);
  newdelwatch::reset();
  return exitcode;
}
