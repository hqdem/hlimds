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

struct BestReplacementDelay {
  float arrivalTime;
};

class SimpleDelayMapper : public CutBaseMapper {
protected:
  void findBest() override;

private:
  std::unordered_map<EntryIndex, BestReplacementDelay> delayVec;

  float findMaxArrivalTime(const std::unordered_set<size_t> &entryIdxs);
  void saveBest(EntryIndex entryIndex,
                const optimizer2::CutExtractor::CutsList &cutsList);
};
} // namespace eda::gate::techmapper
