#ifndef TMEDIA_PALETTE_IO_H
#define TMEDIA_PALETTE_IO_H

#include "palette.h"
#include <filesystem>
#include <string>

Palette read_palette_file(std::filesystem::path path);
Palette read_palette_str(std::string str);

#endif