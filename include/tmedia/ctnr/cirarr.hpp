#ifndef TMEDIA_CIRARR_H
#define TMEDIA_CIRARR_H

#include <cassert>
#include <array>

namespace tmedia {

  /**
   * Self-Overwriting fixed-length array.
  */
  template <typename _Tp, std::size_t _Nm>
  class cirarr {

    typedef _Tp					value_type;
    typedef value_type*			      pointer;
    typedef const value_type*                       const_pointer;
    typedef value_type&                   	      reference;
    typedef const value_type&             	      const_reference;
    typedef value_type*          		      iterator;
    typedef const value_type*			      const_iterator;
    typedef std::size_t                    	      size_type;
    typedef std::ptrdiff_t                   	      difference_type;
    typedef std::reverse_iterator<iterator>	      reverse_iterator;
    typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

    private:
      std::array<_Tp, _Nm> arr;
      std::size_t head;
      std::size_t tail;
    
      reference operator[](std::size_t i);
      reference at(std::size_t i);
      void push_back(const Tp& value);
      std::size_t size();
      std::size_t max_size();

      template <typename Args...>
      void emplace_back(const Tp& value, Args... args);
  };
  

  // template<typename _Tp, std::size_t _Nm>
  // TMEDIA_CPP20_CONSTEXPR inline bool
  // operator==(const CirArr<_Tp, _Nm>& __one, const CirArr<_Tp, _Nm>& __two)
  // { return std::equal(__one.begin(), __one.end(), __two.begin()); }
}



#endif