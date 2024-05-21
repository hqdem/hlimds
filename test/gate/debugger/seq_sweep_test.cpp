//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/seq_checker.h"
#include "gate/model/utils/subnet_random.h"

#include "gtest/gtest.h"

namespace eda::gate::debugger {

std::vector<model::CellSymbol> getSymbols(const model::Subnet &subnet) {
  auto entries = subnet.getEntries();
  std::vector<model::CellSymbol> cells;
  size_t i = 0;
  while (i < entries.size()) {
    cells.push_back(entries[i].cell.getSymbol());
    i += 1 + entries[i].cell.more;
  }
  return cells;
}

TEST(SeqSweepTest, random) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet = 40;

  for (size_t i = 0; i < nSubnet; ++i) {
    const auto &subnet = model::Subnet::get(
        eda::gate::model::randomSubnet(nIn, nOut, nCell, minArity, maxArity));
    auto entries = subnet.getEntries();
    std::vector<size_t> ids;
    std::set<size_t> uniqueIds;
    for (size_t k = (subnet.size() - subnet.getOutNum()); k < subnet.size();
         ++k) {
      ids.push_back(k);
      uniqueIds.insert(k);
    }

    size_t k = 0;
    while (k < ids.size()) {
      size_t currId = ids[k];
      for (size_t l = 0; l < entries[currId].cell.arity; ++l) {
        if (uniqueIds.find(subnet.getLink(currId, l).idx) == uniqueIds.end()) {
          ids.push_back(subnet.getLink(currId, l).idx);
          uniqueIds.insert(subnet.getLink(currId, l).idx);
        }
      }
      ++k;
    }

    std::sort(ids.begin(), ids.end());
    std::vector<model::CellSymbol> symbols(ids.size());
    for (size_t t = 0; t < ids.size(); ++t) {
      symbols[t] = entries[ids[t]].cell.getSymbol();
    }

    const auto &sweepedSubnet = seqSweep(subnet);

    EXPECT_EQ(symbols, getSymbols(sweepedSubnet));
  }
}
} // namespace eda::gate::debugger
