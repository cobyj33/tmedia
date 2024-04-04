#include "palette_io.h"

#include "palette.h"
#include "formatting.h"
#include "funcmac.h"

#include <fstream>
#include <sstream>
#include <fmt/format.h>

Palette read_gimp_gpl_palette(std::istream& stream);

bool is_gimp_gpl_file(std::filesystem::path path);
bool is_gimp_gpl_str(std::string str);
bool test_is_gimp_gpl_stream(std::istream& stream);

const char* GMP_PALETTE_HEADER = "GIMP Palette";

Palette read_palette_file(std::filesystem::path path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error(fmt::format("[{}] Cannot read nonexistent palette "
    "path: {}", FUNCDINFO, path.c_str()));
  }

  if (is_gimp_gpl_file(path)) {
    std::ifstream stream(path);
    return read_gimp_gpl_palette(stream);
  }

  throw std::runtime_error(fmt::format("[{}] Could not read palette for file: "
  "{}", FUNCDINFO, path.c_str()));
}

Palette read_palette_str(std::string str) {
  if (is_gimp_gpl_str(str)) {
    std::istringstream stream(str);
    return read_gimp_gpl_palette(stream);
  }

  throw std::runtime_error(fmt::format("[{}] Could not read palette for "
  "string: {}.", FUNCDINFO, str));
}

bool is_gimp_gpl_file(std::filesystem::path path) {
  std::ifstream stream(path);
  return test_is_gimp_gpl_stream(stream);
}

bool is_gimp_gpl_str(std::string str) {
  std::istringstream stream(str);
  return test_is_gimp_gpl_stream(stream);
}

bool test_is_gimp_gpl_stream(std::istream& stream) {
  std::string line;
  std::getline(stream, line);
  line = str_trim(line, " \r\n");
  return line == GMP_PALETTE_HEADER;
}

Palette read_gimp_gpl_palette(std::istream& stream) {
  Palette palette;
  
  std::string line;
  std::getline(stream, line);
  line = str_trim(line, " \r\n");
  if (line != GMP_PALETTE_HEADER) {
    throw std::runtime_error(fmt::format("[{}] Unrecognized GIMP header",
    FUNCDINFO));
  }

  while (std::getline(stream, line)) {
    line = str_trim(line, " \r\n");
    if (line.empty()) continue;
    if (line[0] == '#') continue; // commented lines
    if (!std::isdigit(line[0])) continue; // commented lines

    RGBColor color;
    std::istringstream lineRead(line);
    lineRead >> color.red >> color.green >> color.blue;
    palette.insert(color); 
  }

  return palette;
}