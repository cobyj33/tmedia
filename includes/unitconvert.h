#ifndef ASCII_VIDEO_UNIT_CONVERSION
#define ASCII_VIDEO_UNIT_CONVERSION

#define GIGABYTES_TO_BYTES 1000000000
#define BYTES_TO_GIGABYTES 1.0 / (GIGABYTES_TO_BYTES)

#define SECONDS_TO_NANOSECONDS 1000000000
#define NANOSECONDS_TO_SECONDS 1.0 / (SECONDS_TO_NANOSECONDS)
#define SECONDS_TO_MILLISECONDS 1000
#define MILLISECONDS_TO_SECONDS 1.0 / (SECONDS_TO_MILLISECONDS)

#define MILLISECONDS_TO_NANOSECONDS 1000000
#define NANOSECONDS_TO_MILLISECONDS 1 / (MILLISECONDS_TO_NANOSECONDS)

#define MINUTES_TO_SECONDS 60
#define SECONDS_TO_MINUTES 1 / (MINUTES_TO_SECONDS)

#define HOURS_TO_SECONDS MINUTES_TO_SECONDS * 60
#define SECONDS_TO_HOURS 1 / (HOURS_TO_SECONDS)

#define DAYS_TO_SECONDS HOURS_TO_SECONDS * 24
#define SECONDS_TO_DAYS 1 / (DAYS_TO_SECONDS)



/* #define MAX_ASCII_IMAGE_WIDTH (long)16 * 16 */
/* #define MAX_ASCII_IMAGE_HEIGHT (long)9 * 16 */
/* #define FRAME_DATA_SIZE MAX_ASCII_IMAGE_WIDTH * MAX_ASCII_IMAGE_HEIGHT */
#endif
