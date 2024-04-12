//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/cut_based/cut_based_mapper.h"

#include <unordered_map>

namespace eda::gate::techmapper {

struct BestReplacementArea{
  double area;
  std::vector<EntryIndex> incomingEntries;
};

class SimpleAreaMapper : public CutBaseMapper {
protected:
  void findBest() override;

private:
  std::unordered_map<EntryIndex, BestReplacementArea> areaVec;

  float calculateArea(const std::unordered_set<uint64_t> &entryIdxs, EntryIndex currnetEntry);
  void saveBest(EntryIndex entryIndex,
                const optimizer2::CutExtractor::CutsList &cutsList);
};
} // namespace eda::gate::techmapper
