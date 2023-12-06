//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

 #include "gate/techoptimizer/cut_based_tech_mapper/strategy/bestReplacement.h"
 #include "gate/optimizer2/cut_extractor.h"
 #include "gate/model2/subnet.h"
 #include "gate/techoptimizer/library/cellDB.h"

 #include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */
namespace eda::gate::tech_optimizer {

  using EntryIndex = uint64_t;
  using CutsList = eda::gate::optimizer2::CutExtractor::CutsList;

  class Strategy {
  public:
    Strategy() {};
    virtual void findBest(EntryIndex entryIndex, const CutsList &CutsList, 
        std::map<EntryIndex, BestReplacement> bestReplacementMap, 
        CellDB &cellDB) = 0;
    //virtual bool checkOpt() = 0;
  };
} // namespace eda::gate::tech_optimizer