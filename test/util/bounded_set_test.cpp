//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/bounded_set.h"

#include "gtest/gtest.h"

#include <ctime>
#include <vector>
#include <unordered_set>

namespace eda::util {

bool equal(const BoundedSet<size_t> &b, const std::unordered_set<size_t> &u) {
  bool result = 1;
  if (b.size() != u.size()) result = 0;
  for (auto i : u)
    result = result && (b.find(i) != b.end());
  return result;
}

TEST(BoundedSetTest, CheckedInsert) {
  std::unordered_set<size_t> set1 {1, 2, 3, 4, 5, 8};
  std::unordered_set<size_t> set2 {10, 11, 1, 0};
  BoundedSet<size_t> bSet1 {5};
  BoundedSet<size_t> bSet2 {6};
  for (auto i : set1) bSet1.insert(i, 1);
  for (auto i : set2) bSet2.insert(i, 1);
  bool result = 1;
  std::unordered_set<size_t> tempSet;
  for (auto i : bSet2) tempSet.insert(i);
  result &= (bSet1.size() == 5);
  result &= (tempSet == set2);
  EXPECT_TRUE(result);
}

TEST(BoundedSetTest, Insert) {
  std::unordered_set<size_t> set1 {1, 2, 3, 4, 5, 8};
  std::unordered_set<size_t> set2 {10, 11, 1, 0, 4, 8};
  BoundedSet<size_t> bSet {12};
  bool result = 1;
  bSet.insert(1);
  bSet.insert(1);
  result &= (std::count(bSet.begin(), bSet.end(), 1) == 1);
  for (auto i : set1) bSet.insert(i);
  for (auto i : set2) bSet.insert(i);
  std::unordered_set<size_t> tempSet;
  for (auto i : set2) set1.insert(i);
  for (auto i : bSet) tempSet.insert(i);
  result &= (tempSet == set1);
  for (auto i : bSet) 
      result &= (std::count(bSet.begin(), bSet.end(), i) == 1);
  EXPECT_TRUE(result);
}

TEST(BoundedSetTest, UnionCheck) {
  bool result = 1;
  BoundedSet<size_t> bSet1 {2, 5};
  BoundedSet<size_t> bSet2 {2, 5};
  result &= (bSet1.unionCheck(bSet2));
  std::unordered_set<size_t> set1 {1, 2, 4, 7, 9, 10};
  std::unordered_set<size_t> set2 {0, 2, 4, 7, 9, 10};
  BoundedSet<size_t> bSet3 {set1, 7};
  BoundedSet<size_t> bSet4 {set2, 7};
  result &= (bSet3.unionCheck(bSet1));
  result &= (bSet3.unionCheck(bSet4));
  result &= (bSet4.unionCheck(bSet4));
  bSet3.merge(bSet4);
  result &= !(bSet3.unionCheck(bSet1));
  EXPECT_TRUE(result);
}

TEST(BoundedSetTest, Merge) {
  bool result = 1;
  std::unordered_set<size_t> set1 {1, 2, 4, 7, 9, 10, 10001,
      112, 12, 3, 55, 88};
  std::unordered_set<size_t> set2 {0, 2, 67, 9, 10001, 11,
      12, 100, 5, 3, 444, 555, 22};
  BoundedSet<size_t> bSet1 {set1, 64};
  BoundedSet<size_t> bSet2 {set2, 64};
  BoundedSet<size_t> bSet3 {set2};
  bSet1.merge(bSet2);
  set1.insert(set2.begin(), set2.end());
  bSet3.merge(bSet1);
  bSet2.merge(bSet1);
  result &= equal(bSet1, set1);
  result &= equal(bSet3, set2);
  result &= (bSet1 == bSet2);
  EXPECT_TRUE(result);
}

TEST(BoundedSetTest, MergeRandomSingletons) {
  srand(time(0));
  bool result = 1;
  const unsigned n = 1000;
  const unsigned merges = 10000;
  const unsigned k = 20;
  size_t r;
  std::vector<BoundedSet<size_t>> vectorBS;
  std::vector<std::unordered_set<size_t>> vectorUS;
  for (unsigned i = 0; i < n; i++) {
    r = rand();
    vectorBS.push_back(BoundedSet<size_t>(k, r));
    vectorUS.push_back( { r } );
  }
  for (unsigned i = 0; i < merges; i++) {
    unsigned first = rand() % n;
    unsigned second = rand() % n;
    if (vectorBS[first].unionCheck(vectorBS[second])) {
      vectorBS[first].merge(vectorBS[second]);
      vectorUS[first].insert(vectorUS[second].begin(),
          vectorUS[second].end());
      result &= equal(vectorBS[first], vectorUS[first]);
    }
  }
  EXPECT_TRUE(result);
}

TEST(BoundedSetTest, MergeWithoutChecks) {
  srand(time(0));
  bool result = 1;
  const unsigned n = 1000;
  const unsigned merges = 10000;
  const unsigned k = 20;
  size_t r;
  std::vector<BoundedSet<size_t>> vectorBS;
  for (unsigned i = 0; i < n; i++) {
    r = rand();
    vectorBS.push_back(BoundedSet<size_t>(k, r));
  }
  for (unsigned i = 0; i < merges; i++) {
    unsigned first = rand() % n;
    unsigned second = rand() % n;
    vectorBS[first].merge(vectorBS[second]);
  }
  for (unsigned i = 0; i < n; i++)
    result &= (vectorBS[i].size() <= k);
  EXPECT_TRUE(result);
}

} // namespace eda::util
