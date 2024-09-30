//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/bounded_set.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <ctime>
#include <vector>
#include <unordered_set>

namespace eda::util {

static bool equal(const BoundedSet<size_t> &b,
                  const std::unordered_set<size_t> &u) {
  bool result = 1;
  if (b.size() != u.size()) result = 0;
  for (auto i : u)
    result = result && (b.find(i) != b.end());
  return result;
}

TEST(BoundedSetTest, Singleton) {
  BoundedSet<size_t> bSet1{4, 1, true};
  BoundedSet<size_t> bSet2{5, 1, false};
  BoundedSet<size_t> bSet3{5, false};
  bSet3.insert(1, true);
  BoundedSet<size_t> bSet4{6, 1, false};
  bSet4.merge(bSet1);
  bSet4.merge(bSet2);
  bSet4.merge(bSet3);
  BoundedSet<size_t> bSet5(bSet1);
  BoundedSet<size_t> bSet6(bSet2);
  BoundedSet<size_t> bSet7(4, { 1 }, true);
  BoundedSet<size_t> bSet8(4, { 1 }, false);
  EXPECT_TRUE(*bSet1.begin() == 1);
  EXPECT_TRUE(*bSet2.begin() == 1);
  EXPECT_TRUE(*bSet3.begin() == 1);
  EXPECT_TRUE(*bSet4.begin() == 1);
  EXPECT_TRUE(*bSet5.begin() == 1);
  EXPECT_TRUE(*bSet6.begin() == 1);
  EXPECT_TRUE(*bSet7.begin() == 1);
  EXPECT_TRUE(*bSet8.begin() == 1);
}

TEST(BoundedSetTest, CheckedInsert) {
  std::unordered_set<size_t> set1{1, 2, 3, 4, 5, 8};
  std::unordered_set<size_t> set2{10, 11, 1, 0};
  BoundedSet<size_t> bSet1{5, false};
  BoundedSet<size_t> bSet2{6, false};
  for (auto i : set1) bSet1.insert(i, 1);
  for (auto i : set2) bSet2.insert(i, 1);
  std::unordered_set<size_t> tempSet;
  for (auto i : bSet2) tempSet.insert(i);
  EXPECT_TRUE(bSet1.size() == 5);
  EXPECT_TRUE(tempSet == set2);
}

TEST(BoundedSetTest, Insert) {
  std::unordered_set<size_t> set1{1, 2, 3, 4, 5, 8};
  std::unordered_set<size_t> set2{10, 11, 1, 0, 4, 8};
  BoundedSet<size_t> bSet{12, false};
  bSet.insert(1);
  bSet.insert(1);
  EXPECT_TRUE(std::count(bSet.begin(), bSet.end(), 1) == 1);
  for (auto i : set1) bSet.insert(i);
  for (auto i : set2) bSet.insert(i);
  std::unordered_set<size_t> tempSet;
  for (auto i : set2) set1.insert(i);
  for (auto i : bSet) tempSet.insert(i);
  EXPECT_TRUE(tempSet == set1);
  for (auto i : bSet) 
    EXPECT_TRUE(std::count(bSet.begin(), bSet.end(), i) == 1);
}

TEST(BoundedSetTest, UnionCheck) {
  BoundedSet<size_t> bSet1{2, 5, true};
  BoundedSet<size_t> bSet2{2, 5, true};
  EXPECT_TRUE(bSet1.unionCheck(bSet2));
  std::unordered_set<size_t> set1{1, 2, 4, 7, 9, 10};
  std::unordered_set<size_t> set2{0, 2, 4, 7, 9, 10};
  BoundedSet<size_t> bSet3{7, set1, false};
  BoundedSet<size_t> bSet4{7, set2, false};
  EXPECT_TRUE(bSet3.unionCheck(bSet1));
  EXPECT_TRUE(bSet3.unionCheck(bSet4));
  EXPECT_TRUE(bSet4.unionCheck(bSet4));
  bSet3.merge(bSet4);
  EXPECT_FALSE(bSet3.unionCheck(bSet1));
}

TEST(BoundedSetTest, Merge) {
  std::unordered_set<size_t> set1
      {1, 2, 4, 7, 9, 10, 10001, 112, 12, 3, 55, 88};
  std::unordered_set<size_t> set2
      {0, 2, 67, 9, 10001, 11, 12, 100, 5, 3, 444, 555, 22};
  BoundedSet<size_t> bSet1{64, set1, false};
  BoundedSet<size_t> bSet2{64, set2, false};
  BoundedSet<size_t> bSet3{32, set2, false};
  bSet1.merge(bSet2);
  set1.insert(set2.begin(), set2.end());
  bSet3.merge(bSet1);
  bSet2.merge(bSet1);
  EXPECT_TRUE(equal(bSet1, set1));
  EXPECT_TRUE(equal(bSet3, set1));
  EXPECT_TRUE(bSet1 == bSet2);
}

TEST(BoundedSetTest, MergeRandomSingletons) {
  srand(time(0));
  const unsigned n = 1000;
  const unsigned merges = 10000;
  const unsigned k = 20;
  size_t r;
  std::vector<BoundedSet<size_t>> vectorBS;
  std::vector<std::unordered_set<size_t>> vectorUS;
  for (unsigned i = 0; i < n; i++) {
    r = rand();
    vectorBS.push_back(BoundedSet<size_t>(k, r, false));
    vectorUS.push_back( { r } );
  }
  for (unsigned i = 0; i < merges; i++) {
    unsigned first = rand() % n;
    unsigned second = rand() % n;
    if (vectorBS[first].unionCheck(vectorBS[second])) {
      vectorBS[first].merge(vectorBS[second]);
      vectorUS[first].insert(vectorUS[second].begin(),
          vectorUS[second].end());
      EXPECT_TRUE(equal(vectorBS[first], vectorUS[first]));
    }
  }
}

TEST(BoundedSetTest, MergeWithoutChecks) {
  srand(time(0));
  const unsigned n = 1000;
  const unsigned merges = 10000;
  const unsigned k = 20;
  size_t r;
  std::vector<BoundedSet<size_t>> vectorBS;
  for (unsigned i = 0; i < n; i++) {
    r = rand();
    vectorBS.push_back(BoundedSet<size_t>(k, r, false));
  }
  for (unsigned i = 0; i < merges; i++) {
    unsigned first = rand() % n;
    unsigned second = rand() % n;
    vectorBS[first].merge(vectorBS[second]);
  }
  for (unsigned i = 0; i < n; i++)
    EXPECT_TRUE(vectorBS[i].size() <= k);
}

} // namespace eda::util
