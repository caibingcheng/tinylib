#ifndef TINY_PCD_H
#define TINY_PCD_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// on linux include mmap
#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tiny_pcd {

namespace {
std::string to_string(const std::string_view &str) { return std::string(str); }

template <typename T> T to_number(const std::string_view &str) {
  // std::from_chars is not available in gcc 7
  if constexpr (std::is_floating_point_v<T>) {
    return std::stod(std::string(str));
  } else {
    return std::stoll(std::string(str));
  }
}

template <typename T, typename T1> static T to_number(const std::string_view &str) {
  return static_cast<T>(reinterpret_cast<const T1 *>(str.data())[0]);
}

template <typename T> static T to_number(const std::string_view &str, const std::string &type) {
  if (type == "int8") {
    return to_number<T, int8_t>(str);
  } else if (type == "uint8") {
    return to_number<T, uint8_t>(str);
  } else if (type == "int16") {
    return to_number<T, int16_t>(str);
  } else if (type == "uint16") {
    return to_number<T, uint16_t>(str);
  } else if (type == "int32") {
    return to_number<T, int32_t>(str);
  } else if (type == "uint32") {
    return to_number<T, uint32_t>(str);
  } else if (type == "int64") {
    return to_number<T, int64_t>(str);
  } else if (type == "uint64") {
    return to_number<T, uint64_t>(str);
  } else if (type == "float32") {
    return to_number<T, float>(str);
  } else if (type == "float64") {
    return to_number<T, double>(str);
  } else {
    throw std::runtime_error("Unknown type " + type);
  }
}

static std::string to_iso_type(const std::string &type, uint32_t size) {
  static const std::vector<std::pair<std::pair<std::string, uint32_t>, std::string>> type_map = {
      {{"I", 1}, "int8"},   {{"I", 2}, "int16"},  {{"I", 4}, "int32"},  {{"I", 8}, "int64"},   {{"U", 1}, "uint8"},
      {{"U", 2}, "uint16"}, {{"U", 4}, "uint32"}, {{"U", 8}, "uint64"}, {{"F", 4}, "float32"}, {{"F", 8}, "float64"},
  };
  for (const auto &[key, value] : type_map) {
    if (type == key.first && size == key.second) {
      return value;
    }
  }
}

bool starts_with(const std::string_view &str, const std::string_view &prefix) {
  return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

void trim(std::string_view &str) {
  while (!str.empty() && std::isspace(str.front())) {
    str.remove_prefix(1);
  }
  while (!str.empty() && std::isspace(str.back())) {
    str.remove_suffix(1);
  }
}
}  // namespace

// ref to: https://pointclouds.org/documentation/tutorials/pcd_file_format.html
class TinyPcd {
 private:
  using strview = std::string_view;

 public:
  enum class PcdType { ASCII, BINARY, UNKNOWN };
  struct Header {
    std::string version;
    std::vector<std::string> field;
    std::vector<uint32_t> size;
    std::vector<std::string> type;
    std::vector<std::string> iso_type;
    std::vector<uint32_t> count;
    uint32_t width;
    uint32_t height;
    std::string view_point;  // what's it?
    uint64_t points;
    PcdType pcd_type;
  };

  class Point {
   public:
    Point(const Header &header, const strview &block) : header_(header), point_(header.field.size()) {
      auto block_start = block;
      for (auto i = 0; i < header.field.size(); ++i) {
        if (header.pcd_type == PcdType::ASCII) {
          if (block_start.empty()) {
            throw std::runtime_error("Parse error, field not found.\n" + std::string(block) + "\n" + header.field[i]);
          }
          const auto pos = block_start.find(' ');
          point_[i] = block_start.substr(0, pos);
          block_start.remove_prefix(pos + 1);
        } else if (header.pcd_type == PcdType::BINARY) {
          const auto item_size = header.size[i] * header.count[i];
          point_[i] = block_start.substr(0, item_size);
          block_start.remove_prefix(item_size);
        }
      }
    }

    std::string data(const std::string &field) const {
      const auto field_idx = field_idx_(field);
      return std::string(point_.at(field_idx));
    }

    double get(const std::string &field) const { return get<double>(field); }

    template <typename T> T get(const std::string &field) const {
      const auto field_idx = field_idx_(field);
      if (header_.pcd_type == PcdType::BINARY) {
        return to_number<T>(point_[field_idx], header_.iso_type[field_idx]);
      } else {
        return std::stod(std::string(point_.at(field_idx)));
      }
    }

   private:
    int64_t field_idx_(const std::string &field) const {
      const auto field_it = std::find(header_.field.begin(), header_.field.end(), field);
      if (field_it == header_.field.end()) {
        throw std::runtime_error("Unknow filed " + field);
      }
      return field_it - header_.field.begin();
    }

   private:
    const Header &header_;
    std::vector<strview> point_;
  };

  class Iterator {
   public:
    Iterator(const Header &header, const strview &blocks) : blocks_(blocks), header_(header) {
      if (header_.pcd_type == PcdType::ASCII) {
        forward_iter_ = [this]() {
          const auto next_line = blocks_.find('\n');
          if (next_line == strview::npos) {
            blocks_ = strview();
          } else {
            blocks_ = blocks_.substr(next_line + 1);
          }
        };
      } else if (header_.pcd_type == PcdType::BINARY) {
        forward_iter_ = [this]() { blocks_ = blocks_.substr(block_size_(header_)); };
      } else {
        throw std::runtime_error("Unknown PCD type.");
      }
    }
    Iterator &operator++() {
      forward_iter_();
      return *this;
    }
    bool operator!=(const Iterator &other) const { return blocks_ != other.blocks_; }
    Point operator*() const { return Point(header_, blocks_); }

   private:
    const Header &header_;
    strview blocks_;
    std::function<void(void)> forward_iter_{nullptr};
  };

 private:
  class Io {
   public:
#ifdef __linux__
    Io(const std::string &filename) {
      file_ = open(filename.c_str(), O_RDONLY);
      if (file_ == -1) {
        throw std::runtime_error("Failed to open the file.");
      }

      struct stat sb;
      if (fstat(file_, &sb) == -1) {
        throw std::runtime_error("Failed to get file size.");
      }

      buffer_ = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, file_, 0);
      if (buffer_ == MAP_FAILED) {
        throw std::runtime_error("Failed to map the file.");
      }

      view_ = strview(static_cast<const char *>(buffer_), sb.st_size);
    }

    ~Io() {
      if (buffer_ != MAP_FAILED) {
        munmap(buffer_, 0);
      }
      if (file_ != -1) {
        close(file_);
      }
    }

   private:
    int file_{-1};
    void *buffer_{MAP_FAILED};

#else
    Io(const std::string &filename) {
      std::ifstream file(filename, std::ios::binary);
      if (!file.is_open()) {
        throw std::runtime_error("Failed to open the file.");
      }

      file.seekg(0, std::ios::end);
      const auto size = file.tellg();
      file.seekg(0, std::ios::beg);

      buffer_.resize(size);
      file.read(buffer_.data(), size);

      view_ = strview(buffer_);
    }

   private:
    std::string buffer_;

#endif

   public:
    strview view() const { return view_; }

   private:
    strview view_;
  };

 public:
  TinyPcd(const std::string &filename) : io_(filename) { parse_header_(io_.view()); }
  Header header() const { return header_; }
  auto size() const { return header_.points; }
  auto fields() const { return header_.field; }
  auto width() const { return header_.width; }
  auto height() const { return header_.height; }
  auto version() const { return header_.version; }

  Iterator begin() const { return Iterator(header_, blocks_); }
  Iterator end() const { return Iterator(header_, strview()); }

  template <typename P> std::vector<Point> filter(P &&pred) const {
    std::vector<Point> result;
    for (const auto &point : *this) {
      if (std::forward<P>(pred)(point)) {
        result.push_back(point);
      }
    }
    return result;
  }

  template <typename T, typename P> T filter(T result, P &&pred) const {
    for (const auto &point : *this) {
      std::forward<P>(pred)(result, point);
    }
    return result;
  }

  Point operator[](int index) const {
    if (index >= header_.points) {
      throw std::runtime_error("Index out of range.");
    }
    if (header_.pcd_type == PcdType::ASCII) {
      if (points_.empty()) {
        points_.emplace_back(begin());
      }
      auto it = points_.back();
      for (int32_t idx = points_.size(); idx <= index; ++idx) {
        ++it;
        points_.emplace_back(it);
      }
      return *(points_[index]);
    } else {
      return Point(header_, blocks_.substr(index * block_size_(header_)));
    }
  }

 private:
  static auto block_size_(const Header &header) -> typename decltype(header.size)::value_type {
    return std::accumulate(header.size.begin(), header.size.end(), 0);
  }

  template <typename T, typename F>
  static bool fill_item_(strview pattern, strview line, T &item, F &&f, bool do_trim = true) {
    if (!starts_with(line, pattern)) {
      return false;
    }
    if (do_trim) {
      line.remove_prefix(pattern.size() + 1);  // remove the space
      trim(line);
    }

    constexpr bool is_vector = std::is_same_v<T, std::vector<std::result_of_t<F(strview)>>>;
    if constexpr (is_vector) {
      item.clear();
      for (auto pos = line.find(' '); pos != strview::npos; pos = line.find(' ')) {
        const auto field = line.substr(0, pos);
        line.remove_prefix(pos + 1);
        item.push_back(std::forward<F>(f)(field));
      }
      item.push_back(std::forward<F>(f)(line));
    }
    if constexpr (!is_vector) {
      item = std::forward<F>(f)(line);
    }

    return true;
  }

  void parse_header_(strview buffer) {
    const auto to_uint32 = [](const strview &str) { return to_number<uint32_t>(str); };
    const auto to_uint64 = [](const strview &str) { return to_number<uint64_t>(str); };
    for (auto pos = buffer.find('\n'); pos != strview::npos; pos = buffer.find('\n')) {
      const auto line = buffer.substr(0, pos);
      buffer.remove_prefix(pos + 1);
      if (line.empty()) {
        continue;
      }
      if (fill_item_("VERSION", line, header_.version, to_string) ||
          fill_item_("FIELDS", line, header_.field, to_string) || fill_item_("TYPE", line, header_.type, to_string) ||
          fill_item_("VIEWPOINT", line, header_.view_point, to_string) ||
          fill_item_("SIZE", line, header_.size, to_uint32) || fill_item_("COUNT", line, header_.count, to_uint32) ||
          fill_item_("WIDTH", line, header_.width, to_uint32) ||
          fill_item_("HEIGHT", line, header_.height, to_uint32) ||
          fill_item_("POINTS", line, header_.points, to_uint64)) {
        // do nothing
      } else if (starts_with(line, "DATA")) {
        blocks_ = buffer;
        if (line.find("ascii") != strview::npos) {
          header_.pcd_type = PcdType::ASCII;
        } else if (line.find("binary") != strview::npos) {
          header_.pcd_type = PcdType::BINARY;
          if (blocks_.size() != header_.points * block_size_(header_)) {
            throw std::runtime_error("Parse error, block size " + std::to_string(block_size_(header_)) +
                                     " but got total size " + std::to_string(blocks_.size()) + ", points " +
                                     std::to_string(header_.points));
          }
        } else {
          throw std::runtime_error("Unknown data type.");
        }

        break;
      }
    }

    if ((!header_.type.empty()) && (header_.size.size() == header_.type.size())) {
      for (auto i = 0; i < header_.type.size(); ++i) {
        header_.iso_type.push_back(to_iso_type(header_.type[i], header_.size[i]));
      }
    }
  }

 private:
  Io io_;
  Header header_;
  strview blocks_;
  mutable std::vector<Iterator> points_;  // only for ascii
};

}  // namespace tiny_pcd

#endif  // TINY_PCD_H
