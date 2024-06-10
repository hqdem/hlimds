//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

#include <unordered_map>

namespace eda::gate::techmapper {

class SimpleAreaMapper : public FuncMapper {
  using SDC = library::SDC;
  using SCLibrary = library::SCLibrary;

  void map(const SubnetID subnetID,
           const SCLibrary &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;

private:
  float dynamicCalculateArea(
          const EntryIndex entryIndex,
          const std::unordered_set<uint64_t> &entryIdxs,
          const SCLibrary &cellDB, Mapping &mapping);
  void saveBest(
         const EntryIndex entryIndex,
         const optimizer::CutExtractor::CutsList &cutsList,
         const SCLibrary &cellDB, Mapping &mapping);
  optimizer::CutExtractor *cutExtractor;
};
} // namespace eda::gate::techmapper