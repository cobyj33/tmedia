#include <tmedia/media/mediaformat.h>

// Chcek tmedia/media/mediaformat.h for more details about why these formats were chosen
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

const char* container_iformat_exts = "matroska,webm,mov,mp4,3gp,3g2,mj2";

/**
 * These extensions are sorted generally in popularity order, so that more
 * popular formats are found first whenever iterating over the elements.
 * 
 * Note that this data should only be used for sorting and filtering quickly, 
 * where accuracy is not the highest importance. In reality, the file extension
 * of a file does not dictate what file it is at all, and these extensions are
 * only conventions. Additionally, many of these file formats are media format
 * containers, which means they can be video, audio, and/or images. If the format
 * described by a file extension could possibly hold different types of
 * multimedia, then the MediaType assigned here will be the most commonly used
 * type of media stored in that format. If there is no "most common", then 
 * std::nullopt should be stored as a last resort.
 * 
 * Also, some formats such as .gif often behave moreso like video rather than
 * audio. However, for filtering sake, most would consider it to be an image
 * format.
 * 
 * Also also, tmedia might doesn't even support all of these file formats.
 * 
 * TL;DR; remember when getting the actual MediaType of a file, it must be
 * actually be opened and probed
*/
const mext_mdata mm_exts[] = { 
// most popular formats  
mext_mdata(".mp3", MediaType::AUDIO), // MP3
mext_mdata(".wav", MediaType::AUDIO), // WAV
mext_mdata(".jpg", MediaType::IMAGE), // JPG
mext_mdata(".jpeg", MediaType::IMAGE), // JPG
mext_mdata(".png", MediaType::IMAGE), // PNG
mext_mdata(".gif", MediaType::IMAGE), // GIF. (I know its technically a video :( ...)
mext_mdata(".webp", MediaType::IMAGE),
mext_mdata(".mp4", MediaType::VIDEO),
mext_mdata(".ogg", MediaType::AUDIO),
mext_mdata(".wav", MediaType::AUDIO),
mext_mdata(".h264", MediaType::VIDEO),
mext_mdata(".mpv", MediaType::AUDIO),
mext_mdata(".mov", MediaType::VIDEO),
mext_mdata(".m4v", MediaType::VIDEO),
mext_mdata(".bmp", MediaType::IMAGE),
mext_mdata(".mkv", MediaType::VIDEO),
mext_mdata(".webm", MediaType::VIDEO),
mext_mdata(".mpeg", MediaType::VIDEO),
mext_mdata(".mpeg1", MediaType::VIDEO),
mext_mdata(".heif", MediaType::IMAGE),
mext_mdata(".heic", MediaType::IMAGE),
mext_mdata(".wma", MediaType::AUDIO), // Windows Media Audio
mext_mdata(".ogv", MediaType::VIDEO), // 
mext_mdata(".aa", MediaType::AUDIO),
mext_mdata(".aac", MediaType::AUDIO),
mext_mdata(".ac3", MediaType::AUDIO),
mext_mdata(".flv", MediaType::VIDEO),
mext_mdata(".tif", MediaType::IMAGE),
mext_mdata(".tiff", MediaType::IMAGE),

mext_mdata(".midi", MediaType::AUDIO),
mext_mdata(".mid", MediaType::AUDIO),
mext_mdata(".flac", MediaType::AUDIO),
mext_mdata(".cda",  MediaType::AUDIO), // maybe?
mext_mdata(".aif", MediaType::AUDIO),
mext_mdata(".mpg", MediaType::VIDEO),
mext_mdata(".m1v", MediaType::VIDEO),
mext_mdata(".m4a", MediaType::AUDIO),
mext_mdata(".m4b", MediaType::AUDIO), // audiobook format
mext_mdata(".m4p", MediaType::AUDIO),
mext_mdata(".3ga", MediaType::AUDIO),
mext_mdata(".3gp", MediaType::VIDEO),
mext_mdata(".3gpp", MediaType::VIDEO),
mext_mdata(".3g2", MediaType::VIDEO),
mext_mdata(".mj2", MediaType::VIDEO),  // motion jpeg is not an image no matter what you tell me
mext_mdata(".wmv", MediaType::VIDEO),
mext_mdata(".vob", MediaType::VIDEO),
mext_mdata(".mp2", MediaType::AUDIO),
mext_mdata(".mp1", MediaType::AUDIO),
mext_mdata(".m2a", MediaType::AUDIO),
mext_mdata(".m1a", MediaType::AUDIO),
mext_mdata(".avi", MediaType::VIDEO),
mext_mdata(".apng", MediaType::IMAGE),
mext_mdata(".mng", MediaType::VIDEO),
mext_mdata(".divx", MediaType::VIDEO),
mext_mdata(".rmvb", MediaType::VIDEO),
mext_mdata(".evo", MediaType::VIDEO),
mext_mdata(".mov", MediaType::VIDEO),
mext_mdata(".mpe", MediaType::VIDEO),
mext_mdata(".asf", MediaType::VIDEO),

// ogg aliases
mext_mdata(".ogm", MediaType::VIDEO), 
mext_mdata(".ogx", MediaType::VIDEO), 
mext_mdata(".oga", MediaType::VIDEO),
mext_mdata(".opus", MediaType::VIDEO),

// jpeg aliases
mext_mdata(".jpe", MediaType::IMAGE),
mext_mdata(".jif", MediaType::IMAGE),
mext_mdata(".jfif", MediaType::IMAGE),
mext_mdata(".jfi", MediaType::IMAGE),
mext_mdata(".jp2", MediaType::IMAGE),
mext_mdata(".j2k", MediaType::IMAGE),
mext_mdata(".jpf", MediaType::IMAGE),
mext_mdata(".jpm", MediaType::IMAGE),
mext_mdata(".jpg2", MediaType::IMAGE),
mext_mdata(".j2c", MediaType::IMAGE),
mext_mdata(".jpc", MediaType::IMAGE),
mext_mdata(".jpx", MediaType::IMAGE),

//heic aliases
mext_mdata(".heifs", MediaType::IMAGE),
mext_mdata(".heics",  MediaType::IMAGE),
mext_mdata(".avci",  MediaType::IMAGE),

mext_mdata(".dib", MediaType::IMAGE), // device independent bitmap
mext_mdata(".pbm", MediaType::IMAGE),
mext_mdata(".pgm", MediaType::IMAGE),
mext_mdata(".ppm", MediaType::IMAGE),
mext_mdata(".pnm", MediaType::IMAGE),
mext_mdata(".qt", MediaType::VIDEO),

mext_mdata(".avcs", MediaType::VIDEO),
mext_mdata(".HIF", MediaType::IMAGE),
mext_mdata(".psp", MediaType::IMAGE),
mext_mdata(".m4b", MediaType::AUDIO), // book
mext_mdata(".ism", MediaType::VIDEO),
mext_mdata(".ismv", MediaType::VIDEO),
mext_mdata(".isma", MediaType::AUDIO),


mext_mdata(".raw", MediaType::AUDIO), // pcm audio?

// flv aliases
mext_mdata(".fla", MediaType::VIDEO),
mext_mdata(".f4v", MediaType::VIDEO),
mext_mdata(".f4a", MediaType::VIDEO),
mext_mdata(".f4b", MediaType::VIDEO),
mext_mdata(".f4p", MediaType::VIDEO),
// flash aliases
mext_mdata(nullptr, MediaType::VIDEO) }; // keep the last string null terminated!