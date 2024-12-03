#ifndef TINY_MATCH_H
#define TINY_MATCH_H

// c++17 and above
static_assert(__cplusplus >= 201703L, "C++17 or above is required.");

#include <array>
#include <functional>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

namespace tiny_utility {

struct placeholder_t {};
constexpr placeholder_t placeholder;

template <typename A, typename B>
struct can_compare : std::false_type {};
template <typename A>
struct can_compare<A, A> : std::true_type {};
template <typename A, typename B, std::enable_if_t<std::is_same_v<decltype(std::declval<A>() == std::declval<B>()), bool>, int> = 0>
struct can_compare<A, B> : std::true_type {};

template <typename A, typename B>
inline constexpr bool can_compare_v = can_compare<A, B>::value;


template <typename Key, typename Action> class Case {
 public:
  Case(const Key &key, Action &&action) : key_(key), action_(std::forward<Action>(action)) {}
  Case(Key &&key, Action &&action) : key_(std::forward<Key>(key)), action_(std::forward<Action>(action)) {}

  template <typename QueryKey> bool operator()(const QueryKey &key) {
    using decay_key = std::remove_cv_t<std::remove_reference_t<Key>>;
    using decay_query_key = std::remove_cv_t<std::remove_reference_t<QueryKey>>;
    constexpr bool is_placeholder = can_compare_v<decay_key, decay_query_key>;
    constexpr bool can_compare = std::
    constexpr bool can_invoke = std::is_invocable_v<Action, decay_key>;
    constexpr auto find = [](const auto &key, const auto &query_key) {
      if constexpr (std::is_same_v<decay_key, std::vector> || std::is_same_v<decay_key, std::list> ||
                    std::is_same_v<decay_key, std::set> || std::is_same_v<decay_key, std::unordered_set>) {
        return key.find(query_key) != key.end();
      } else {
        return false;
      }
    };

    const bool can_match = is_placeholder || (can_compare && key == key_) || (!can_compare && find(key_, key));
    if (can_match) {
      if constexpr (can_invoke) {
        action_(key);
      } else {
        action_();
      }
      return true;
    } else {
      return false;
    }
  }

 private:
  Key key_;
  Action action_;
};

template <typename T> inline bool operator==(const T &lhs, const placeholder_t &) { return true; }

template <typename T> inline bool operator==(const T &lhs, const std::vector<T> &rhs) {
  return std::find(rhs.begin(), rhs.end(), lhs) != rhs.end();
}

template <typename T> inline bool operator==(const T &lhs, const std::list<T> &rhs) {
  return std::find(lhs.begin(), lhs.end(), rhs) != lhs.end();
}

template <typename T> inline bool operator==(const T &lhs, const std::set<T> &rhs) {
  return rhs.find(lhs) != rhs.end();
}

template <typename T> inline bool operator==(const T &lhs, const std::unordered_set<T> &rhs) {
  return rhs.find(lhs) != rhs.end();
}

inline bool operator==(const char *lhs, const std::string &rhs) { return std::string(lhs) == rhs; }

template <typename... Case> class Match {
 public:
  Match(Case &&...cases) : cases_(std::forward<Case>(cases)...) {}

  template <typename Key> void operator()(const Key &key) {
    std::apply([&key](auto &...c) { (c(key) || ...); }, cases_);
  }

 private:
  std::tuple<Case...> cases_;
};

template <typename Key, typename Action> inline auto operator|(Key &&key, Action &&action) {
  return Case(std::forward<Key>(key), std::forward<Action>(action));
}

template <typename Key, typename Action> inline auto operator|(const Key &key, Action &&action) {
  return Case<Key, Action>(key, std::forward<Action>(action));
}

template <typename Action> inline auto operator|(const char *key, Action &&action) {
  return Case(std::string(key), std::forward<Action>(action));
}

template <typename... Case> inline auto match(Case &&...cases) { return Match{std::forward<Case>(cases)...}; }

}  // namespace tiny_utility
#endif  // TINY_MATCH_H
