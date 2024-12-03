#ifndef TINY_BITMAP_H
#define TINY_BITMAP_H

#include <algorithm>
#include <memory>
#include <utility>

template <typename T> class BitMap {
 public:
  BitMap(const std::initializer_list<T> &list) {
    const auto max_value = *std::max_element(list.begin(), list.end());
    const auto size = max_value * 8 / 8;
    data_ = std::make_shared<uint8_t[]>(size);
    for (const auto &value : list) {
      data_[value] = 1;
    }
  }

  bool operator[](const T &index) const { return data_[index] == 1; }
  uint8_t &operator[](const T &index) { return data_[index]; }

 private:
  std::shared_ptr<uint8_t[]> data_;
};

#endif  // TINY_BITMAP_H