#ifndef TMEDIA_CONSTANTS_H
#define TMEDIA_CONSTANTS_H

// constants.h - Various constants that are shared around tmedia
// that are the basis for how the program interacts with itself. All of these
// constants should generally be used in application code and not in library
// code (i.e., base PixelData functions shouldn't assume images are under
// MAX_FRAME_WIDTH and things like that)

// When adding constants to this file, make sure that they have corresponding
// necessary static assertions at the end of this file to guarantee behavior
// expected by the rest of tmedia.

// MIN_RENDER_COLS and MIN_RENDER_LINES serve as the minimum dimensions
// necessary for any rendering to be done by tmedia. Instead of in dedicated
// terminal emulators, These extremely low width and height values usually can
// occur moreso in text editors or other applications where the terminal is a
// GUI pane.

static constexpr int MIN_RENDER_COLS = 2;
static constexpr int MIN_RENDER_LINES = 2;

// Pixel Aspect Ratio - account for tall rectangular shape of terminal
// characters
static constexpr int PAR_WIDTH = 2;
static constexpr int PAR_HEIGHT = 5;


// Do note that all MAX_FRAME constants are scaled by the 'pixel' aspect ratio
// of terminal characters. Therefore, a 16:9 aspect ratio is 16:9 in the
// terms of the number of pixels, but not in the terms of those actual pixel's
// sizes

// Also, do note that the width is scaled by PAR_HEIGHT and the height is
// scaled by PAR_WIDTH. I found that this helps inverse the distortion caused
// on videos by rectangular 'pixels'

static constexpr int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PAR_HEIGHT;
static constexpr int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PAR_WIDTH;
static constexpr double MAX_FRAME_ASPECT_RATIO = static_cast<double>(MAX_FRAME_ASPECT_RATIO_WIDTH) / static_cast<double>(MAX_FRAME_ASPECT_RATIO_HEIGHT);

/**
 * MAX_FRAME_WIDTH and MAX_FRAME_HEIGHT denote the maximum image sizes
 * producable by a MediaFetcher instance. I decided that they should be in
 * this constants.h file rather than in tmedia/media/mediafetcher.h since they
 * are used by other parts of tmedia for storing buffers large enough for
 * generated frames, and could be used later in other parts of the code
 * as limits to image input sizes and whatnot.
 *
 * 640 for MAX_FRAME_WIDTH is quite arbitrary, but I chose it since
 * width of 640 at a 4:3 aspect ratio ends up around 480p. Additionally,
 *
 * I find that past a width of 640 characters,
 * the terminal starts to stutter terribly on most terminal emulators, and CPU
 * usage becomes extremely high, so we
 * just bound the image to this amount.
 *
 * This number can just be configured to any maximum amount wanted during
 * compilation. Currently, this value is not changeable at runtime, but there
 * are considerations for allowing that behavior.
 *
 * MAX_FRAME_HEIGHT is derived from MAX_FRAME_WIDTH and MAX_FRAME_ASPECT_RATIO,
 * and comes to around 144.
 */
static constexpr int MAX_FRAME_WIDTH = 640;
static constexpr int MAX_FRAME_HEIGHT = static_cast<int>(static_cast<double>(MAX_FRAME_WIDTH) / MAX_FRAME_ASPECT_RATIO);

// Static assertions

static_assert(MIN_RENDER_COLS > 0);
static_assert(MIN_RENDER_LINES > 0);

static_assert(PAR_WIDTH > 0);
static_assert(PAR_HEIGHT > 0);

static_assert(MAX_FRAME_ASPECT_RATIO_WIDTH > 0);
static_assert(MAX_FRAME_ASPECT_RATIO_HEIGHT > 0);
static_assert(MAX_FRAME_ASPECT_RATIO > 0.0);

static_assert(MAX_FRAME_WIDTH > 0);
static_assert(MAX_FRAME_HEIGHT > 0);

#endif