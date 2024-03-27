//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techmapper/mapper/cut_base/cut_base_mapper.h"

#include <unordered_map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

using Subnet = eda::gate::model::Subnet;

namespace eda::gate::tech_optimizer {
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
} // namespace eda::gate::tech_optimizer