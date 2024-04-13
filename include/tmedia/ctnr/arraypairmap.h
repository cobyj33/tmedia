#ifndef TMEDIA_ARRAY_PAIR_MAP_H
#define TMEDIA_ARRAY_PAIR_MAP_H

#include <array>
#include <map>
#include <cassert>

namespace tmedia {

  #if 0

  template<typename _Key, typename _Tp, std::size_t _Nm>
  struct ArrayPairMapTraits;
  {
    typedef _Tp _Type[_Nm];
    typedef _K _Key[_Nm];
    typedef __is_swappable<_Tp> _Is_swappable;
    typedef __is_nothrow_swappable<_Tp> _Is_nothrow_swappable;

    static constexpr _Tp&
    _S_ref(const _Type& __t, std::size_t __n) noexcept
    { return const_cast<_Tp&>(__t[__n]); }

    static constexpr _Tp*
    _S_ptr(const _Type& __t) noexcept
    { return const_cast<_Tp*>(__t); }
  };

  template<typename _Key, typename _Tp>
  struct ArrayPairMapTraits<_Key, _Tp, 0>
  {
    struct _Type { };
    typedef true_type _Is_swappable;
    typedef true_type _Is_nothrow_swappable;

    static constexpr _Tp&
    _S_ref(const _Type&, std::size_t) noexcept
    { return *static_cast<_Tp*>(nullptr); }

    static constexpr _Tp*
    _S_ptr(const _Type&) noexcept
    { return nullptr; }
  };

  template <typename _Key, typename _Tp, std::size_t _Nm>
  typedef std::array<std::pair<_Key, _Tp>, _Nm> ArrayPairMap; 


  template <typename _Key, typename _Tp, std::size_t _Nm, typename _Compare = std::less<_Key>>
  class ArrayPairMap {
    typedef _Key					key_type;
    typedef _Tp					mapped_type;
    typedef std::pair<const _Key, _Tp>		value_type;
    typedef _Compare					key_compare;

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

    // No explicit construct/copy/destroy for aggregate type.

    typedef ArrayPairMapTraits<_Key, _Tp, _Nm> _AT_Type;
    typename _AT_Type::_Type                         keys;
    typename _AT_Type::_Type                         values;

    pointer
      data() noexcept
      { return _AT_Type::_S_ptr(_M_elems); }

    // DR 776.
    _GLIBCXX20_CONSTEXPR void
    fill(const value_type& __u)
    { std::fill_n(begin(), size(), __u); }

    _GLIBCXX20_CONSTEXPR void
    swap(array& __other)
    noexcept(_AT_Type::_Is_nothrow_swappable::value)
    { std::swap_ranges(begin(), end(), __other.begin()); }

    // Iterators.
    _GLIBCXX17_CONSTEXPR iterator
    begin() noexcept
    { return iterator(data()); }

    _GLIBCXX17_CONSTEXPR const_iterator
    begin() const noexcept
    { return const_iterator(data()); }

    _GLIBCXX17_CONSTEXPR iterator
    end() noexcept
    { return iterator(data() + _Nm); }

    _GLIBCXX17_CONSTEXPR const_iterator
    end() const noexcept
    { return const_iterator(data() + _Nm); }

    _GLIBCXX17_CONSTEXPR reverse_iterator
    rbegin() noexcept
    { return reverse_iterator(end()); }

    _GLIBCXX17_CONSTEXPR const_reverse_iterator
    rbegin() const noexcept
    { return const_reverse_iterator(end()); }

    _GLIBCXX17_CONSTEXPR reverse_iterator
    rend() noexcept
    { return reverse_iterator(begin()); }

    _GLIBCXX17_CONSTEXPR const_reverse_iterator
    rend() const noexcept
    { return const_reverse_iterator(begin()); }

    _GLIBCXX17_CONSTEXPR const_iterator
    cbegin() const noexcept
    { return const_iterator(data()); }

    _GLIBCXX17_CONSTEXPR const_iterator
    cend() const noexcept
    { return const_iterator(data() + _Nm); }

    _GLIBCXX17_CONSTEXPR const_reverse_iterator
    crbegin() const noexcept
    { return const_reverse_iterator(end()); }

    _GLIBCXX17_CONSTEXPR const_reverse_iterator
    crend() const noexcept
    { return const_reverse_iterator(begin()); }

    // Capacity.
    constexpr size_type
    size() const noexcept { return _Nm; }

    constexpr size_type
    max_size() const noexcept { return _Nm; }

    _GLIBCXX_NODISCARD constexpr bool
    empty() const noexcept { return size() == 0; }

      // Element access.
      _GLIBCXX17_CONSTEXPR reference
      operator[](size_type __n) noexcept
      {
    assert(__n < this->size());
    return _AT_Type::_S_ref(_M_elems, __n);
        }

        constexpr const_reference
        operator[](size_type __n) const noexcept
        {
  #if __cplusplus >= 201402L
    assert(__n < this->size());
  #endif
    return _AT_Type::_S_ref(_M_elems, __n);
        }

        _GLIBCXX17_CONSTEXPR reference
        at(size_type __n)
        {
    if (__n >= _Nm)
      std::__throw_out_of_range_fmt(__N("array::at: __n (which is %zu) "
                ">= _Nm (which is %zu)"),
            __n, _Nm);
    return _AT_Type::_S_ref(_M_elems, __n);
        }

        constexpr const_reference
        at(size_type __n) const
        {
    // Result of conditional expression must be an lvalue so use
    // boolean ? lvalue : (throw-expr, lvalue)
    return __n < _Nm ? _AT_Type::_S_ref(_M_elems, __n)
      : (std::__throw_out_of_range_fmt(__N("array::at: __n (which is %zu) "
                  ">= _Nm (which is %zu)"),
              __n, _Nm),
        _AT_Type::_S_ref(_M_elems, 0));
        }

        _GLIBCXX17_CONSTEXPR reference
        front() noexcept
        {
          assert(this->size() > 0);
          return *begin();
        }

        constexpr const_reference
        front() const noexcept
        {
  #if __cplusplus >= 201402L
    assert(this->size() > 0);
  #endif
    return _AT_Type::_S_ref(_M_elems, 0);
        }

        _GLIBCXX17_CONSTEXPR reference
        back() noexcept
        {
    assert(this->size() > 0);
    return _Nm ? *(end() - 1) : *end();
        }

        constexpr const_reference
        back() const noexcept
        {
  #if __cplusplus >= 201402L
    assert(this->size() > 0);
  #endif
    return _Nm ? _AT_Type::_S_ref(_M_elems, _Nm - 1)
              : _AT_Type::_S_ref(_M_elems, 0);
        }

        _GLIBCXX17_CONSTEXPR pointer
        data() noexcept
        { return _AT_Type::_S_ptr(_M_elems); }

        _GLIBCXX17_CONSTEXPR const_pointer
        data() const noexcept
        { return _AT_Type::_S_ptr(_M_elems); }
  };

  #endif

}

#endif