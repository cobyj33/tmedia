#ifndef TMEDIA_SMALLVEC_H
#define TMEDIA_SMALLVEC_H

#include <array>
#include <vector>

namespace tmedia {

  template <typename _Tp, std::size_t _Nm>
  class svector {
    std::size_t size;
    std::array<_Tp, _Nm> arr;
    std::vector<_Tp> vec;

    typedef _Tp value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef value_type*        iterator;
    typedef const value_type*	const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t   difference_type;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    
  };
}

#endif