#ifndef ASCII_VIDEO_TERM_COLOR
#define ASCII_VIDEO_TERM_COLOR
/**
 * @file termcolor.h
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief NCurses routines to pre-load color maps and to quickly fetch colors based on RGB Values
 * @version 0.1
 * @date 2023-03-15
 * 
 * @copyright MIT License (c) 2023 (see LICENSE)
 */

#include <vector>

#include "color.h"

/**
 * @brief Initialize ncurses's color palette as grayscale 
 */
void ncurses_initialize_grayscale_color_palette();

/**
 * @brief Initialize ncurses's color palette as a full color palette 
 * In it's current implementation, this function will load 7^3 (216) colors into the ncurses color palette
 */
void ncurses_initialize_color_palette();

/**
 * @brief Find the closest registered ncurses color pair integer to the inputted RGBColor.
 * 
 * Color pairs are used to apply attributes to printed terminal colors
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @returns The closest registered ncurses color pair attribute index
 */
int get_closest_ncurses_color_pair(const RGBColor& input);

/**
 * @brief Find the closest registered ncurses color integer to the inputted RGBColor.
 * 
 * Colors are used to create ncurses color pairs, which are then used to apply attributes to printed terminal colors
 * Generally, if printing is required, get_closest_ncurses_color_pair is a recommended alternative  
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @returns The closest registered ncurses color pair attribute index
 */
int get_closest_ncurses_color(const RGBColor& input);

/**
 * @brief Find the closest registered ncurses grayscale color pair integer to the inputted RGBColor.
 * 
 * Color pairs are used to apply attributes to printed terminal colors
 * @note The rgb color does not have to be converted to grayscale in order to work with this function, as this function WILL convert the color itself
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @note This function will return a valid grayscale color pair if either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run. However, ncurses_initialize_grayscale_color_palette() will allow this function to return a much more accurate grayscale representation of the inputted color
 * @returns The closest registered ncurses color pair attribute index
 */
int get_closest_ncurses_grayscale_color_pair(const RGBColor& input);

/**
 * @brief Find the closest registered ncurses color integer to the inputted RGBColor.
 * 
 * Colors are used to create ncurses color pairs, which are then used to apply attributes to printed terminal colors
 * Generally, if printing is required, get_closest_ncurses_grayscale_color_pair is a recommended alternative  
 * @note The RGBColor does not have to be converted to grayscale in order to work with this function, as this function WILL convert the color itself
 * @note This function should only be used after either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run
 * @note This function will return a valid grayscale color pair if either ncurses_initialize_color_palette() or ncurses_initialize_grayscale_color_palette() is run. However, ncurses_initialize_grayscale_color_palette() will allow this function to return a much more accurate grayscale representation of the inputted color
 * @returns The closest registered ncurses color pair attribute index
 */
int get_closest_ncurses_grayscale_color(const RGBColor& input);

#endif