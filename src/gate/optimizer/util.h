//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/walker.h"
#include "gate/simulator/simulator.h"

/**
 * \brief Method used for rewriting.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using GateID = eda::gate::model::GNet::GateId;
  using Gate = eda::gate::model::Gate;
  using Cut = CutStorage::Cut;
  using Simulator = simulator::Simulator;

  void substitute(GateID cutFor, const CutStorage::Cut &cut, GNet *subsNet, GNet *net);

  bool fakeSubstitute(GateID cutFor, const Cut &cut, GNet *subsNet, GNet *net);

  uint64_t getTruthTable(GateID cutFor, const Cut &cut, GNet *net);
} // namespace eda::gate::optimizer

