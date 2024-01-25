//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techoptimizer/mapper/baseMapper.h"

#include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

namespace eda::gate::tech_optimizer {
class CutBaseMapper : BaseMapper {
public:
  void map(SubnetID subnetID,
           CellDB &cellDB,
           std::map<EntryIndex, BestReplacement> *bestReplacementMap) override;

protected:
  virtual void findBest(SubnetID subnetID,
                        CellDB &cellDB,
                        CutExtractor &cutExtractor,
                        std::map<EntryIndex, BestReplacement>
                            *bestReplacementMap) = 0;
};
} // namespace eda::gate::tech_optimizer