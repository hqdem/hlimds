//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

#include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

using Subnet = eda::gate::model::Subnet;

namespace eda::gate::tech_optimizer {
class SimpleAreaMapper : public CutBaseMapper {

protected:
  void findBest() override;
private:

  float calculateArea(const std::unordered_set<uint64_t> &entryIdxs);
  void saveBest(EntryIndex entryIndex,
                const optimizer2::CutExtractor::CutsList &cutsList);

  void addInputToTheMap(EntryIndex entryIndex);
  void addZeroToTheMap(EntryIndex entryIndex);
  void addOneToTheMap(EntryIndex entryIndex);
  void addOutToTheMap(EntryIndex entryIndex,
                      model::Subnet::Cell &cell);
};
} // namespace eda::gate::tech_optimizer