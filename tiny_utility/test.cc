#include <gtest/gtest.h>

#include "tiny_match.h"
#include "tiny_range.h"
#include "tiny_zip.h"
#include "bitmap.h"

#include <algorithm>
#include <list>

TEST(TinyRange, Range1) {
  using namespace tiny_utility;
  std::vector<int32_t> vec{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  auto count = 0;
  for (auto i : range(vec.size())) {
    ASSERT_EQ(i, count++);
  }
  ASSERT_EQ(count, 10);
}

TEST(TinyRange, Range2) {
  using namespace tiny_utility;
  auto count = 0;
  for (auto i : range(10, 20)) {
    ASSERT_EQ(i, count + 10);
    ++count;
  }
  ASSERT_EQ(count, 10);
}

TEST(TinyRange, Zip1) {
  using namespace tiny_utility;

  std::list<int32_t> ls{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  std::vector<int32_t> vec(ls.begin(), ls.end());
  std::vector<int32_t> vec2(ls.begin(), ls.end());
  std::reverse(vec2.begin(), vec2.end());

  auto count = 0;
  for (auto [i, j, k, l] : zip(range(vec.size()), ls, vec, vec2)) {
    ASSERT_EQ(i, count);
    ASSERT_EQ(j, 10 * (count + 1));
    ASSERT_EQ(k, 10 * (count + 1));
    ASSERT_EQ(l, 100 - 10 * count);
    ++count;
  }
  ASSERT_EQ(count, 10);
}

TEST(TinyRange, Zip2) {
  using namespace tiny_utility;
  auto count = 0;
  for (auto [i] : zip(range(10))) {
    ASSERT_EQ(i, count++);
  }
  ASSERT_EQ(count, 10);
}

TEST(TinyRange, Zip3) {
  using namespace tiny_utility;
  auto z = zip(range(10), range(20));
  auto count = 0;
  for (auto it = z.begin(); it != z.end(); ++it) {
    const auto [i, j] = *it;
    ASSERT_EQ(i, count);
    ASSERT_EQ(j, count);
    ++count;
  }
  ASSERT_EQ(count, 10);
}

TEST(TinyRange, Match1) {
  using namespace tiny_utility;
  match(
      1 | [](const auto &i) { ASSERT_EQ(i, 1); },  // no line break
      2 | [](const auto &i) { ASSERT_EQ(i, 2); },  // no line break
      3 | [](const auto &i) { ASSERT_EQ(i, 3); },  // no line break
      4 | [](const auto &i) { ASSERT_EQ(i, 4); }   // no line break
      )(4);
}

TEST(TinyRange, Match2) {
  using namespace tiny_utility;
  auto match_op = match(
      1 | [](const auto &i) { ASSERT_EQ(i, 1); },                             // no line break
      2 | [](const auto &i) { ASSERT_EQ(i, 2); },                             // no line break
      3 | [](const auto &i) { ASSERT_EQ(i, 3); },                             // no line break
      std::vector<int32_t>{4, 5, 6} | [](const auto &i) { ASSERT_NE(i, 4); }  // no line break
  );
  match_op(5);
}

TEST(TinyRange, Match3) {
  using namespace tiny_utility;
  auto match_op = match(
      "1" | [](const auto &i) { ASSERT_EQ(i, "1"); },                              // no line break
      "2" | [](const auto &i) { ASSERT_EQ(i, "2"); },                              // no line break
      "3" | [](const auto &i) { ASSERT_EQ(i, "3"); },                              // no line break
      std::set<std::string>{"5", "6"} | [](const auto &i) { ASSERT_NE(i, "4"); },  // no line break
      placeholder | []() { ASSERT_TRUE(false); }                      // no line break
  );
  // for (const auto &key : std::vector<std::string>{"1", "2", "3", "4" , "5", "6"}) {
  for (const auto &key : std::vector<std::string>{"1", "2", "3", "5", "6"}) {
    match_op(key);
  }
}

TEST(BitMap, BitMap1) {
  using namespace tiny_utility;
  BitMap<uint32_t> bm{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  for (auto i = 1; i <= 10; ++i) {
    ASSERT_TRUE(bm[i]);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
