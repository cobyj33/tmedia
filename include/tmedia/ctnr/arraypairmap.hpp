#ifndef TMEDIA_ARRAY_PAIR_MAP_H
#define TMEDIA_ARRAY_PAIR_MAP_H

#include <array>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <algorithm>

namespace tmedia {

  template <typename _K, typename _Tp, std::size_t _Nm, typename _Compare = std::less<_K>>
  class ArrayPairMap {
    typedef _K key_type;
    typedef _Tp	value_type;
    typedef std::pair<const key_type, value_type> pair_type;
    typedef std::pair<const key_type&, value_type&> ref_pair_type;
    typedef _Compare key_compare;

    typedef std::array<key_type, _Nm> key_array;
    typedef std::array<value_type, _Nm> value_array;

    typedef typename key_array::pointer key_pointer;
    typedef typename key_array::const_pointer const_key_pointer;
    typedef typename value_array::pointer value_pointer;
    typedef typename value_array::const_pointer const_value_pointer;

    typedef typename key_array::reference key_reference;
    typedef typename key_array::const_reference const_key_reference;
    typedef typename value_array::reference value_reference;
    typedef typename value_array::const_reference const_value_reference;

    typedef typename key_array::iterator  key_iterator;
    typedef typename key_array::const_iterator const_key_iterator;
    typedef typename value_array::iterator value_iterator;
    typedef typename value_array::const_iterator const_value_iterator;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    typedef std::reverse_iterator<key_iterator> reverse_key_iterator;
    typedef std::reverse_iterator<const_key_iterator> const_reverse_key_iterator;

    typedef std::reverse_iterator<value_iterator> reverse_value_iterator;
    typedef std::reverse_iterator<const_value_iterator> const_reverse_value_iterator;

    #if 0
    class pair_iterator : public std::iterator<
                                std::random_access_iterator_tag, // iterator_category
                                ref_pair_type,                    // value_type
                                std::ptrdiff_t> {
        friend class ArrayPairMap<_K, _Tp>;
        ArrayPairMap<_K, _Tp>& map;
        size_type index;
    public:
        explicit pair_iterator() {}
        explicit pair_iterator(size_t index): index(index) {}
        iterator& operator++() { index++; return *this; }
        iterator operator++(int) { index++; return *this; }
        iterator operator+=(std::size_t val) { index += val; return *this; }
        bool operator==(iterator other) const { return this->idx == other.idx; }
        bool operator!=(iterator other) const { return !(*this == other); }
        ref_pair_type operator*() const { return std::make_pair<_K, _Tp>(map); }
        const ref_pair_type operator*() const { return std::make_pair<_K, _Tp>(map.); }
    };

    typedef const pair_iterator const_pair_iterator;

    typedef std::reverse_iterator<pair_iterator> reverse_pair_iterator;
    typedef std::reverse_iterator<const_pair_iterator> const_reverse_pair_iterator;
    #endif

    key_array m_keys;
    value_array m_values;
    size_type m_size;

  public:
    TMEDIA_CPP17_CONSTEXPR
    ArrayPairMap() {
      this->m_size = 0;
    }

    TMEDIA_CPP17_CONSTEXPR
    ArrayPairMap(const std::initializer_list<pair_type>& kv_pairs) {
      if (kv_pairs.size() > _Nm)
        throw std::out_of_range("[tmedia::ArrayPairMap] kv_pairs larger than array size.");

      size_type i = 0;      
      for (const pair_type& pair : kv_pairs) {
        this->m_keys[i] = pair.first;
        this->m_values[i] = pair.second;
        i++;
      }
      this->m_size = i;
    }

    // Value Iterators

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

    TMEDIA_CPP17_CONSTEXPR const_value_iterator
    vcbegin() const noexcept
    { return this->m_values.cbegin(); }

    TMEDIA_CPP17_CONSTEXPR const_value_iterator
    vcend() const noexcept
    { return this->m_values.cbegin() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_value_iterator
    vcrbegin() const noexcept
    { return const_reverse_iterator(this->cend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_value_iterator
    vcrend() const noexcept
    { return const_reverse_iterator(this->cbegin()); }


    // Key Iterators

    

    TMEDIA_CPP17_CONSTEXPR key_iterator
    kbegin() noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kbegin() const noexcept
    { return this->m_keys.begin(); }

    TMEDIA_CPP17_CONSTEXPR key_iterator
    kend() noexcept
    { return this->m_keys.begin() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kend() const noexcept
    { return this->m_keys.begin() + this->size(); }

    TMEDIA_CPP17_CONSTEXPR reverse_key_iterator
    krbegin() noexcept
    { return std::reverse_iterator(this->kend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_key_iterator
    krbegin() const noexcept
    { return std::reverse_iterator(this->kend()); }

    TMEDIA_CPP17_CONSTEXPR reverse_key_iterator
    krend() noexcept
    { return std::reverse_iterator(this->kbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_key_iterator
    krend() const noexcept
    { return std::reverse_iterator(this->kbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kcbegin() const noexcept
    { return const_iterator(this->kbegin()); }

    TMEDIA_CPP17_CONSTEXPR const_key_iterator
    kcend() const noexcept
    { return const_iterator(this->kend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_key_iterator
    kcrbegin() const noexcept
    { return const_reverse_iterator(this->kcend()); }

    TMEDIA_CPP17_CONSTEXPR const_reverse_key_iterator
    kcrend() const noexcept
    { return const_reverse_iterator(this->kcbegin()); }


    // Capacity.
    constexpr size_type
    size() const noexcept { return this->m_size; }

    constexpr size_type
    max_size() const noexcept { return _Nm; }

    _GLIBCXX_NODISCARD constexpr bool
    empty() const noexcept { return size() == 0; }

    TMEDIA_CPP17_CONSTEXPR bool
    contains(const key_type& key) const {
      return std::find(this->kbegin(), this->kend(), key) != this->kend();
    }

    TMEDIA_CPP17_CONSTEXPR int
    count(const key_type& key) const {
      return static_cast<int>(this->contains(key));
    }

    TMEDIA_CPP17_CONSTEXPR void
    add(const key_type& key, const value_type& val) {
      this->m_keys[this->m_size++] = key;
      this->m_values[this->m_size++] = val;
      this->m_size++;
    }


    // Element access.
    // TMEDIA_CPP17_CONSTEXPR const value_type&
    // operator[](const key_type& key) const noexcept
    // {
    //   for (size_type i = 0; i < this->size(); i++) {
    //     if (this->m_keys[i] == key) return this->m_values[i];
    //   }
    //   throw std::out_of_range("[ArrayPairMap] could not find key");
    // }

    TMEDIA_CPP17_CONSTEXPR const value_type&
    at(const key_type& key) const {
      for (size_type i = 0; i < this->size(); i++) {
        if (this->m_keys[i] == key) return this->m_values[i];
      }
      throw std::out_of_range("[ArrayPairMap] could not find key");
    }

    // Element access.
    TMEDIA_CPP17_CONSTEXPR const value_type&
    operator[](const key_type& key) const
    { return this->at(key); }

    // TMEDIA_CPP17_CONSTEXPR const value_type&
    // at(const key_type& key) const noexcept { return this->operator[key]; }

    TMEDIA_CPP17_CONSTEXPR ref_pair_type
    front() noexcept
    {
      assert(this->size() > 0);
      return {this->m_keys[0], this->m_values[0]};
    }

    TMEDIA_CPP17_CONSTEXPR ref_pair_type
    back() noexcept
    {
      assert(this->size() > 0);
      return {this->m_keys[this->size() - 1], this->m_values[this->size() - 1]};
    }
  };

  // #endif

}

#endif
