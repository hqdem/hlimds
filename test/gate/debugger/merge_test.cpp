//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/seq_checker.h"
#include "gate/model2/examples.h"
#include "gate/model2/utils/subnet_random.h"

#include "gtest/gtest.h"

namespace eda::gate::debugger {

std::vector<int> getIdx(const Subnet &subnet) {
  std::vector<int> idxs;
  size_t i = 0;
  auto entr = subnet.getEntries();
  while (i < entr.size()) {
    idxs.push_back(i);
    i += 1 + entr[i].cell.more;
  }
  return idxs;
}

bool compareSubnets(const Subnet &sub1, const Subnet &sub2,
                    std::unordered_map<size_t, std::vector<size_t>> &classes) {
  CellToCell maps;
  for (const auto &eq : classes) {
    for (const auto &id : eq.second) {
      maps[id] = eq.first;
    }
  }

  std::vector<int> idxs1 = getIdx(sub1);
  std::vector<int> idxs2 = getIdx(sub2);

  if (idxs1.size() != idxs2.size()) {
    return false;
  }

  auto entr = sub2.getEntries();
  size_t i = 0;
  while (i < idxs1.size()) {
    if (maps.find(idxs1[i]) != maps.end()) {
      if (entr[idxs2[i]].cell.refcount) {
        return false;
      }
    }
    ++i;
  }

  return true;
}

TEST(MergeSpeculativeTest, custom1) {
  Subnet &subnet = Subnet::get(eda::gate::model::makeAndOrXor());
  std::unordered_map<size_t, std::vector<size_t>> classes = {{2, {3, 4}}};
  const Subnet &mergedSubnet = merge(subnet, classes, true);
  EXPECT_TRUE(compareSubnets(subnet, mergedSubnet, classes));
}

TEST(MergeSpeculativeTest, custom2) {
  Subnet &subnet = Subnet::get(eda::gate::model::make4AndOr());
  std::unordered_map<size_t, std::vector<size_t>> classes = {{2, {3}},
                                                             {6, {4, 5}}};
  const Subnet &mergedSubnet = merge(subnet, classes, true);

  EXPECT_TRUE(compareSubnets(subnet, mergedSubnet, classes));
}

TEST(MergeConstantTest, custom1) {
  Subnet &subnet = Subnet::get(eda::gate::model::make2Latches());
  std::vector<size_t> classes = {3, 5};
  const Subnet &mergedSubnet = merge(subnet, CellSymbol::ZERO, classes);

  EXPECT_TRUE(mergedSubnet.size() == 6);
  EXPECT_TRUE(mergedSubnet.getInNum() == 2);
  EXPECT_TRUE(mergedSubnet.getOutNum() == 3);
  EXPECT_TRUE(mergedSubnet.getEntries()[1].cell.isFlipFlop());
  EXPECT_TRUE(mergedSubnet.getEntries()[2].cell.getSymbol() ==
              CellSymbol::ZERO);
}

TEST(MergeConstantTest, custom2) {
  Subnet &subnet = Subnet::get(eda::gate::model::makeLatche());
  std::vector<size_t> classes = {2, 6};
  const Subnet &mergedSubnet = merge(subnet, CellSymbol::ZERO, classes);

  EXPECT_TRUE(mergedSubnet.size() == 2);
  EXPECT_TRUE(mergedSubnet.getInNum() == 0);
  EXPECT_TRUE(mergedSubnet.getOutNum() == 1);
  EXPECT_TRUE(mergedSubnet.getEntries()[0].cell.getSymbol() ==
              CellSymbol::ZERO);
}

} // namespace eda::gate::debugger
