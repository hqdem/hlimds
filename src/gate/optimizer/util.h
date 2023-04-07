//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/cone_visitor.h"
#include "gate/optimizer/links_add_counter.h"
#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/links_clean_counter.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/substitute_visitor.h"
#include "gate/optimizer/walker.h"
#include "gate/simulator/simulator.h"

/**
 * \brief Methods used for rewriting.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using GateID = eda::gate::model::GNet::GateId;
  using Gate = eda::gate::model::Gate;
  using Cut = CutStorage::Cut;
  using Simulator = simulator::Simulator;
  using BoundGNet = RWDatabase::BoundGNet;

  void substitute(GateID cutFor, const std::unordered_map<GateID, GateID> &map, GNet *subsNet, GNet *net);

  int fakeSubstitute(GateID cutFor, const std::unordered_map<GateID, GateID> &map, GNet *subsNet, GNet *net);

  uint64_t getTruthTable(GateID cutFor, const Cut &cut, GNet *net);
} // namespace eda::gate::optimizer

