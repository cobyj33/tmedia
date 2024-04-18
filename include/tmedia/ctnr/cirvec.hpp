#ifndef TMEDIA_CIRVEC_H
#define TMEDIA_CIRVEC_H


#include <vector>

template <typename _Tp>
class Cirvec {
  std::vector<_Tp> m_elems;
  std::size_t m_head;
  std::size_t m_tail;

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

#endif