//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/library/sdc_manager.h"
#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

#include <unordered_map>

namespace eda::gate::techmapper {

struct BestReplacementDelay {
  float arrivalTime;
};

class SimpleDelayMapper : public FuncMapper {
  using SCLibrary = library::SCLibrary;
  using SDC = library::SDC;
  virtual ~SimpleDelayMapper() = default;
  void map(const SubnetID subnetID,
           const SCLibrary &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;

private:
  std::unordered_map<EntryIndex, BestReplacementDelay> delayVec;

  float findMaxArrivalTime(const std::unordered_set<size_t> &entryIdxs);
  void saveBest(const EntryIndex entryIndex,
                const optimizer::CutExtractor::CutsList &cutsList,
                const SCLibrary &cellDB, Mapping &mapping);
  optimizer::CutExtractor *cutExtractor;
};
} // namespace eda::gate::techmapper
