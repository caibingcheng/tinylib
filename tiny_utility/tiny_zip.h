#ifndef TINY_ZIP_H
#define TINY_ZIP_H

// c++17 and above
static_assert(__cplusplus >= 201703L, "C++17 or above is required.");

#include <cstdint>
#include <functional>
#include <iterator>
#include <tuple>

namespace tiny_utility {

template <typename... Res> class Zip {
 public:
  class iterator {
    template <typename T> using res_iterator = decltype(std::declval<T>().begin());
    template <typename T> using res_reference = decltype(*std::declval<res_iterator<T>>());

    using res_iterator_pack = std::tuple<res_iterator<Res>...>;
    using reference = std::tuple<res_reference<Res>...>;

   public:
    iterator(const res_iterator<Res> &...it) : it_(it...) {}
    iterator(res_iterator<Res> &&...it) : it_(std::forward<res_iterator<Res>>(it)...) {}
    iterator &operator++() {
      std::apply([](auto &...it) { ((++it), ...); }, it_);
      return *this;
    }
    bool operator!=(const iterator &other) const {
      return all_not_equal(other, std::make_index_sequence<sizeof...(Res)>{});
    }
    reference operator*() {
      return std::apply([](auto &...it) { return reference(*it...); }, it_);
    }

   private:
    template <std::size_t... I> bool all_not_equal(const iterator &other, std::index_sequence<I...>) const {
      return ((std::get<I>(it_) != std::get<I>(other.it_)) && ...);
    }

   private:
    res_iterator_pack it_;
  };

 public:
  Zip(Res &&...res) : begin_(std::forward<Res>(res).begin()...), end_(std::forward<Res>(res).end()...) {}

  auto begin() const { return begin_; }
  auto end() const { return end_; }

 private:
  iterator begin_;
  iterator end_;
};

template <typename... Res> inline auto zip(Res &&...res) { return Zip<Res...>(std::forward<Res>(res)...); }

}  // namespace tiny_utility

#endif  // TINY_ZIP_H
