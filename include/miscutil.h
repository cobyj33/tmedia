#ifndef TMEDIA_MISC_UTIL_H
#define TMEDIA_MISC_UTIL_H

#include <map>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <utility>

template <typename T>
std::vector<std::string_view> get_strmap_keys(const std::map<std::string_view, T>& map) {
  std::vector<std::string> keys;
  for (typename std::map<std::string, T>::iterator it = map.begin(); it != map.end(); it++) {
    keys.push_back(it->first);
  }
  return keys;
}

template <typename T, typename F>
std::vector<T> transform_vec(std::vector<T>& vec, F&& unary_func) {
  std::transform(vec.begin(), vec.end(), vec.begin(), unary_func);
  return vec;
}

#endif