/*
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "kitty/kitty.hpp"

#include "gate/model2/subnet.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/techmapper/strategy/strategy_example.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"
#include "gate/techoptimizer/library/cellDB.h"
#include "util/logging.h"

#include "gtest/gtest.h"
#include "map"

using CutExtractor = eda::gate::optimizer2::CutExtractor;
using Subnet = eda::gate::model::Subnet;
using SubnetID = eda::gate::model::SubnetID;
using EntryIndex = uint64_t;

namespace eda::gate::tech_optimizer {

TEST(TechmapStrategyTest, MinDelay) {

  CellDB cellDB = getSimpleCells();

  Subnet subnet = Subnet::get(subnet1());

  CutExtractor cutExtractor(&subnet, 6);

  std::map<EntryIndex, BestReplacement> bestReplacementMap;

  eda::gate::model::Array<Subnet::Entry> enstries = subnet.getEntries();
  for (EntryIndex i = 0; i < std::size(enstries); i++) {
    
    //init strategy
    //strategy->findBest(i, cutExtractor.getCuts(i), 
    //    bestReplacementMap, cellDB);
    auto cell = enstries[i].cell;
    i += cell.more - 1;
  }
}

}*/
