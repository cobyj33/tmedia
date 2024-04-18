#ifndef TMEDIA_PAIR_MAP_H
#define TMEDIA_PAIR_MAP_H

#include <array>
#include <vector>
#include <utility>
#include <map>

namespace tmedia {

  // #if 0
  template <typename _Key, typename _Tp, std::size_t _Nm, typename _Compare = std::less<_Key>>
  class ArrayPairMap;

  template <typename _Key, typename _Tp, std::size_t _Nm,
  typename _Compare = std::less<_Key>,
  typename _Alloc = std::allocator<std::pair<const _Key, _Tp> >>
  class PairMap {

    std::vector<_Key> m_keys;
    std::vector<_Tp> m_values;

    

  };

  // #endif


}


#endif