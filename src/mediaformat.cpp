#include "mediaformat.h"

// Chcek mediaformat.h for more details about why these formats were chosen
// to be explicitly listed.

/**
 * For some reason, ffmpeg can play text files back like videos with this
 * teletype format? We don't want that, that's weird
*/
const char* banned_iformat_names = "tty";

const char* image_iformat_names = "image2,png_pipe,webp_pipe";
const char* audio_iformat_names = "wav,ogg,mp3,flac";
const char* video_iformat_names = "flv";

const char* image_iformat_exts= "jpg,jpeg,png,webp";
const char* audio_iformat_exts= "mp3,flac,wav,ogg";
const char* video_iformat_exts= "flv";
