#include <tmedia/ffmpeg/ffmpeg_error.h> 
 
#include <string>
 
std::string av_strerror_string(int errnum) {
  char errBuf[512];
  av_strerror(errnum, errBuf, 512);
  return std::string(errBuf);
}
