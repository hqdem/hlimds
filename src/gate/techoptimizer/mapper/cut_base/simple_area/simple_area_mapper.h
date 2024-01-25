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
class SimpleAreaMapper : CutBaseMapper {

protected:
  void findBest(SubnetID subnetID,
                        CellDB &cellDB,
                        CutExtractor &cutExtractor,
                        std::map<EntryIndex, BestReplacement>
                        *bestReplacementMap) override;
private:
  std::map<EntryIndex, BestReplacement> *bestReplacementMap;
  float calculateArea(const std::unordered_set<uint64_t> &entryIdxs,
                      const SubnetID &subID,
                      const CellDB &cellDb);
  void saveBest(EntryIndex entryIndex, const CutsList &cutsList,
                 CellDB &cellDB,
                 SubnetID subnetID);
  void forwardPass(SubnetID subnetID);

  void addInputToTheMap(EntryIndex entryIndex);
  void addZeroToTheMap(EntryIndex entryIndex);
  void addOneToTheMap(EntryIndex entryIndex);
  void addOutToTheMap(EntryIndex entryIndex,
                      model::Subnet::Cell &cell);
};
} // namespace eda::gate::tech_optimizer