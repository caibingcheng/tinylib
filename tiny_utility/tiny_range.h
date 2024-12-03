#ifndef TINY_RANGE_H
#define TINY_RANGE_H

// c++17 and above
static_assert(__cplusplus >= 201703L, "C++17 or above is required.");

#include <iterator>

namespace tiny_utility {

template <typename T> class Range {
 public:
  class iterator {
   public:
    iterator(T value) : value_(value) {}
    iterator &operator++() {
      ++value_;
      return *this;
    }
    bool operator!=(const iterator &other) const { return value_ != other.value_; }
    const T &operator*() { return value_; }

   private:
    T value_;
  };

 public:
  Range(T end) : Range(T(), end) {}
  Range(T begin, T end) : begin_(begin), end_(end) {}

  auto begin() const { return begin_; }
  auto end() const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template <typename T> inline auto range(T end) { return Range<T>(end); }
template <typename T> inline auto range(T begin, T end) { return Range<T>(begin, end); }

}  // namespace tiny_utility

#endif  // TINY_RANGE_H
