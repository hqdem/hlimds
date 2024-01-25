//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"

#include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

namespace eda::gate::tech_optimizer {
 class BaseMapper {
 public:
  virtual void map(SubnetID subnetID,
                   CellDB &cellDB,
                   std::map<EntryIndex, BestReplacement> *bestReplacementMap) = 0;

  virtual ~BaseMapper() = default;
 };
} // namespace eda::gate::tech_optimizer