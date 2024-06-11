#ifndef TMEDIA_PIXEL_DATA_H
#define TMEDIA_PIXEL_DATA_H

#include <tmedia/image/color.h>

#include <vector>
#include <cstdint>

extern "C" {
struct AVFrame;
}

/**
 * All PixelData API functions are written so that instead of returning a
 * PixelData instance, they take in a reference to an existing PixelData
 * instance as their output destination. This allows callers to choose how to
 * allocate their PixelData instance and allows callers to choose whether they
 * want to reuse an already allocated buffer as the result of some function's
 * effects. This is particularly useful since it ensures that large allocations
 * can be mitigated by callers through reuse. If any new functions are written
 * to be used with a PixelData struct, they should also be written in this
 * same format for this reason.
 *
 * PixelData is also a simple type so that any function
 *
 * While encapsulating state may have been a good choice for ensuring these
 * invariants, I found it as unnecessary and trying to fix a problem that
 * doesn't exist, as well as limiting when trying to use a PixelData in a way
 * unintended by initial design. Just use the provided functions and the
 * invariants will never break.
 *
 * Invariants of a PixelData struct:
 * PixelData::pixels.size() == PixelData::m_width * PixelData::m_height
*/
class PixelData {
  public:
    std::vector<RGB24> pixels;
    int m_width;
    int m_height;
};

/**
 * Whenever a PixelData struct is first created, it should be initialized with
 * pixdata_setnewdims or pixdata_initgray. This is to ensure that the PixelData
 * is not filled with garbage data and is initialized to a given color.
 *
 * If it is known that a PixelData instance will never exceed a certain size,
 * The PixelData instance will be reused often, and the maximum size is
 * reasonable for allocating all at once, it is recommended to just initialize
 * the PixelData instance to it's largest size, so that all other operations
 * on the PixelData instance will cause no reallocation on the internal vector.
 */

/**
 * A function to reliably change the dimensions of a given PixelData struct.
 *
 * @param width width must be greater than or equal to zero
 * @param height height must be greater than or equal to zero
 *
 * Note that if the current PixelData already has color information within,
 * this information will not be erased. This function only resizes the internal
 * PixelData::pixels vector and sets the width and height accordingly.
 *
 * If the new width and height are larger than the
*/
void pixdata_setnewdims(PixelData& dest, int width, int height);

/**
 * @param width width must be greater than or equal to zero
 * @param height height must be greater than or equal to zero
 * @param g The grayscale value to initialize all pixels in the PixelData to
*/
void pixdata_initgray(PixelData& dest, int width, int height, std::uint8_t g);

/**
 *
 *
 */
void pixdata_copy(PixelData& dest, PixelData& src);

/**
 *
 * @param dest The destination PixelData to copy the frame data from src into
* @param src src cannot be null
*/
void pixdata_from_avframe(PixelData& dest, AVFrame* src);

/**
 * Draw a vertical line on the given PixelData on a given column with a given
 * color.
 */
void vertline(PixelData& dest, int row1, int row2, int col, const RGB24 color);

/**
 * Draw a horizontal line on the given PixelData on a given column with a given
 * color.
 */
void horzline(PixelData& dest, int col1, int col2, int row, const RGB24 color);

#endif
