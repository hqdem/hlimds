//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/optimizer_visitor.h"
#include "gate/optimizer/strategy/track_strategy.h"
#include "gate/optimizer/tracker_visitor.h"
#include "gate/optimizer/walker.h"

#include <queue>

/**
 * \brief Method for net optimization base on rewriting.
 */
namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using GateID = eda::gate::model::GNet::GateId;
  using Gate = eda::gate::model::Gate;
  using Cut = std::unordered_set<GateID>;

  /**
   * Optimizes a net on based on rewriting.
   * @param net Net to be optimized.
   * @param cutSize Number of nodes in a cut.
   * @param optimizer Implementation of optimization strategy.
   */
  void optimize(GNet *net, int cutSize, OptimizerVisitor &&optimizer);

  /**
   * Optimizes a net on based on rewriting with logging optimization steps.
   * @param net Net to be optimized.
   * @param cutSize Number of nodes in a cut.
   * @param subCatalog Path to the folder for outputting log information.
   * @param optimizer Implementation of optimization strategy.
   */
  void
  optimizePrint(GNet *net, int cutSize, const std::filesystem::path &subCatalog,
                OptimizerVisitor &&optimizer);

  /**
   * Finds all cuts of a given size.
   * @param cutSize Max number of nodes in a cut.
   * @param net Net to search in.
   * @return Struct with all found cuts.
   */
  CutStorage findCuts(int cutSize, GNet *net);

} // namespace eda::gate::optimizer
