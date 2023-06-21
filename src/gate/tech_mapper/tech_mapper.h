//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/optimizer_visitor.h"
#include "gate/optimizer/walker.h"
#include "gate/simulator/simulator.h"
#include "gate/tech_mapper/strategy/replacement_cut.h"
#include "gate/tech_mapper/tech_map_visitor.h"

#include <queue>

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using GateID = eda::gate::model::GNet::GateId;
  using Gate = eda::gate::model::Gate;
  using Cut = std::unordered_set<GateID>;

  void techMap(GNet *net, int cutSize, RWDatabase &rwdb, 
      TechMapVisitor&& techMapper, ReplacementVisitor&& replacer);

  void techMapPrinter(GNet *net, int cutSize, RWDatabase &rwdb, 
      TechMapVisitor&& techMapper, ReplacementVisitor&& replacer,
      const std::filesystem::path &subCatalog);

  CutStorage findCuts(int cutSize, GNet *net);

} // namespace eda::gate::optimizer
