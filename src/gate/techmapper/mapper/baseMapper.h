//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techmapper/library/SDC.h"
#include "gate/techmapper/library/cellDB.h"
#include "gate/techmapper/mapper/bestReplacement.h"

#include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

namespace eda::gate::tech_optimizer {
class BaseMapper {
public:
  void mapping(SubnetID subnetID,
           CellDB *cellDB,
           SDC &sdc,
           std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap);

  virtual ~BaseMapper() = default;
protected:
  CellDB *cellDB;
  SubnetID subnetID;
  SDC sdc;
  std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap;

  virtual void baseMap() = 0;
 };
} // namespace eda::gate::tech_optimizer