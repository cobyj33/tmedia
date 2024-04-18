#ifndef TMEDIA_ARRAY_PAIR_MAP_H
#define TMEDIA_ARRAY_PAIR_MAP_H

#include <array>
#include <map>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <tmedia/util/defines.h>
#include <utility>

#include <type_traits>

namespace tmedia {

  // #if 0

  // template<typename _K, typename _Tp, std::size_t _Nm>
  // struct ArrayPairMapTraits {
  //   typedef _Tp _Type[_Nm];
  //   typedef _K _Key[_Nm];
  //   typedef __is_swappable<_Tp> _Is_swappable;
  //   typedef __is_nothrow_swappable<_Tp> _Is_nothrow_swappable;

  //   static constexpr _Tp&
  //   _S_vref(const _Type& __t, std::size_t __n) noexcept
  //   { return const_cast<_Tp&>(__t[__n]); }

  //   static constexpr _K&
  //   _S_kref(const _Key& __k, std::size_t __n) noexcept
  //   { return const_cast<_K&>(__k[__n]); }

  //   static constexpr _Tp*
  //   _S_vptr(const _Type& __t) noexcept
  //   { return const_cast<_Tp*>(__t); }

  //   static constexpr _K*
  //   _S_kptr(const _Key& __k, std::size_t __n) noexcept
  //   { return const_cast<_K*>(__k); }
  // };

  // template<typename _K, typename _Tp>
  // struct ArrayPairMapTraits<_K, _Tp, 0>
  // {
  //   struct _Type { };
  //   typedef true_type _Is_swappable;
  //   typedef true_type _Is_nothrow_swappable;

  //   static constexpr _Tp&
  //   _S_vref(const _Type& __t, std::size_t __n) noexcept
  //   { return *static_cast<_Tp*>(nullptr); }

  //   static constexpr _K&
  //   _S_kref(const _Key& __k, std::size_t __n) noexcept
  //   { return *static_cast<_K*>(nullptr); }

  //   static constexpr _Tp*
  //   _S_vptr(const _Type& __t) noexcept
  //   { return const_cast<_Tp*>(nullptr); }

  //   static constexpr _K*
  //   _S_kptr(const _Key& __k, std::size_t __n) noexcept
  //   { return const_cast<_K*>(nullptr); }
  // };


  template <typename _K, typename _Tp, std::size_t _Nm, typename _Compare = std::less<_K>>
  class ArrayPairMap {
    typedef _K key_type;
    typedef _Tp	value_type;
    typedef std::pair<const key_type&, value_type&> pair_type;
    typedef _Compare key_compare;

    typedef std::array<key_type, _Nm> key_array;
    typedef std::array<value_type, _Nm> value_array;

    typedef key_array::pointer key_pointer;
    typedef key_array::const_pointer const_key_pointer;
    typedef value_array::pointer value_pointer;
    typedef value_array::const_pointer const_value_pointer;

    typedef key_array::reference key_reference;
    typedef key_array::const_reference const_key_reference;
    typedef value_array::reference value_reference;
    typedef value_array::const_reference const_value_reference;

    typedef key_array::iterator  key_iterator;
    typedef key_array::const_iterator const_key_iterator;
    typedef value_array::iterator value_iterator;
    typedef value_array::const_iterator const_value_iterator;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    typedef std::reverse_iterator<key_iterator> reverse_key_iterator;
    typedef std::reverse_iterator<const_key_iterator> const_reverse_key_iterator;

    typedef std::reverse_iterator<value_iterator> reverse_value_iterator;
    typedef std::reverse_iterator<const_value_iterator> const_reverse_value_iterator;

    template <typename _K, typename _Tp>
    class pair_iterator : public std::iterator<
                                std::random_access_iterator_tag, // iterator_category
                                pair_type,                    // value_type
                                std::ptrdiff_t> {
        ArrayPairMap<_K, _Tp>& map;
        size_type index;
    public:
        explicit iterator() {}
        explicit iterator(size_t index): index(index) {}
        iterator& operator++() { index++; return *this; }
        iterator operator++(int) { index++; return *this; }
        iterator operator+=(std::size_t val) { index += val; return *this; }
        bool operator==(iterator other) const { return this->idx == other.idx; }
        bool operator!=(iterator other) const { return !(*this == other); }
        reference operator*() const { return std::make_pair<_K, _Tp>(map); }
        const reference operator*() const { return std::make_pair<_K, _Tp>(map.); }
    };

    typedef const pair_iterator const_pair_iterator;

    typedef std::reverse_iterator<pair_iterator> reverse_pair_iterator;
    typedef std::reverse_iterator<const_pair_iterator> const_reverse_pair_iterator;

    // No explicit construct/copy/destroy for aggregate type.

    key_array m_keys;
    value_array m_values;
    size_type m_size;

    ArrayPairMap() {}

    ArrayPairMap(std::initializer_list<pair_type> kv_pairs) {
      if (kv_pairs.size() > _Nm) 
        throw std::out_of_range("[tmedia::ArrayPairMap] kv_pairs larger than array size.");

      size_type i = 0;      
      for (; i < kv_pairs.size(); i++) {
        this->m_keys[i] = kv_pairs[i];
        this->m_values[i] = kv_pairs[i];
      }
      this->m_size = i;
    }

    TMEDIA_CPP17_CONSTEXPR key_pointer
    keys() noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR const_key_pointer
    keys() const noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR value_pointer
    values() noexcept
    { return this->m_values.begin(); }

    TMEDIA_CPP17_CONSTEXPR const_value_pointer
    values() const noexcept
    { return this->m_values.begin(); }

    // Iterators.
    TMEDIA_CPP17_CONSTEXPR value_iterator
    vbegin() noexcept
    { return this->m_values.begin(); }

    TMEDIA_CPP17_CONSTEXPR const_value_iterator
    vbegin() const noexcept
    { return this->m_values.begin(); }

    TMEDIA_CPP17_CONSTEXPR value_iterator
    vend() noexcept
    { return this->m_values.begin() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR const_value_iterator
    vend() const noexcept
    { return this->m_values.begin() + this->size(); }

    // Iterators.
    TMEDIA_CPP17_CONSTEXPR key_iterator
    kbegin() noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kbegin() const noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR key_iterator
    kend() noexcept
    { return this->m_keys.end() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kend() const noexcept
    { return this->m_keys.end() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR reverse_value_iterator
    vrbegin() noexcept
    { return std::reverse_iterator(this->vend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_value_iterator
    vrbegin() const noexcept
    { return std::reverse_iterator(this->vend()); }

    TMEDIA_CPP17_CONSTEXPR reverse_value_iterator
    vrend() noexcept
    { return std::reverse_iterator(this->vbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_value_iterator
    vrend() const noexcept
    { return std::reverse_iterator(this->vbegin()); }

    TMEDIA_CPP17_CONSTEXPR reverse_iterator
    krbegin() noexcept
    { return std::reverse_iterator(this->kend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    krbegin() const noexcept
    { return std::reverse_iterator(this->kend()); }

    TMEDIA_CPP17_CONSTEXPR reverse_iterator
    krend() noexcept
    { return std::reverse_iterator(this->kbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    krend() const noexcept
    { return std::reverse_iterator(this->kbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_iterator
    vcbegin() const noexcept
    { return this->m_values.cbegin(); }

    TMEDIA_CPP17_CONSTEXPR const_iterator
    vcend() const noexcept
    { return this->m_values.cbegin() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    vcrbegin() const noexcept
    { return const_reverse_iterator(cbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    vcrend() const noexcept
    { return const_reverse_iterator(begin()); }

    TMEDIA_CPP17_CONSTEXPR const_iterator
    kcbegin() const noexcept
    { return const_iterator(data()); }

    TMEDIA_CPP17_CONSTEXPR const_iterator
    kcend() const noexcept
    { return const_iterator(data() + this->size()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    kcrbegin() const noexcept
    { return const_reverse_iterator(end()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_iterator
    kcrend() const noexcept
    { return const_reverse_iterator(begin()); }

    // Capacity.
    constexpr size_type
    size() const noexcept { return this->m_size; }

    constexpr size_type
    max_size() const noexcept { return _Nm; }

    _GLIBCXX_NODISCARD constexpr bool
    empty() const noexcept { return size() == 0; }

    // Element access.
    TMEDIA_CPP17_CONSTEXPR pair_type
    operator[](size_type __n) noexcept
    {
      assert(__n < this->size());
      return {this->m_keys[i], this->m_values[i]};
    }

    operator[](size_type __n) const noexcept
    {
      #if __cplusplus >= 201402L
      assert(__n < this->size());
      #endif
      return {this->m_keys[i], this->m_values[i]};
    }

    TMEDIA_CPP17_CONSTEXPR pair_type
    at(size_type __n)
    {
      if (__n >= this->size())
        std::out_of_range("[ArrayPairMap::at] given size n out of range");
      return {this->m_keys[i], this->m_values[i]};
    }

    TMEDIA_CPP17_CONSTEXPR reference
    front() noexcept
    {
      assert(this->size() > 0);
      return *begin();
    }

    constexpr const_reference
    front() const noexcept
    {
      assert(this->size() > 0);
      return _AT_Type::_S_ref(_M_elems, 0);
    }

    TMEDIA_CPP17_CONSTEXPR reference
    back() noexcept
    {
      assert(this->size() > 0);
      return _Nm ? *(end() - 1) : *end();
    }

    constexpr const_reference
    back() const noexcept
    {
      assert(this->size() > 0);
      return _Nm ? _AT_Type::_S_ref(_M_elems, _Nm - 1)
                : _AT_Type::_S_ref(_M_elems, 0);
    }
  };

  // #endif

}

#endif