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
  using GateId = eda::gate::model::GNet::GateId;
  using Gate = eda::gate::model::Gate;
  using Cut = std::unordered_set<GateId>;

  /**
   * Optimizes a net on based on rewriting.
   * @param net Net to be optimized.
   * @param cutSize Number of nodes in a cut.
   * @param optimizer Implementation of optimization strategy.
   * @param maxCutsNumber Maximum number of cuts for a single node.
   * To avoid restriction CutsFindVisitor::ALL_CUTS can be used.
   */
  void optimize(GNet *net, unsigned int cutSize, OptimizerVisitor &&optimizer,
                unsigned int maxCutsNumber = CutsFindVisitor::ALL_CUTS);

  /**
   * Optimizes a net on based on rewriting with logging optimization steps.
   * @param net Net to be optimized.
   * @param cutSize Number of nodes in a cut.
   * @param subCatalog Path to the folder for outputting log information.
   * @param optimizer Implementation of optimization strategy.
   * @param maxCutsNumber Maximum number of cuts for a single node.
   * To avoid restriction CutsFindVisitor::ALL_CUTS can be used.
   */
  void optimizePrint(GNet *net, unsigned int cutSize,
                     const std::filesystem::path &subCatalog,
                     OptimizerVisitor &&optimizer,
                     unsigned int maxCutsNumber = CutsFindVisitor::ALL_CUTS);

  /**
   * Method that finds all cuts of a given size.
   * @param net Net to search in.
   * @param cutSize Max number of nodes in a cut.
   * @param maxCutsNumber Maximum number of cuts for a single node.
   * To avoid restriction CutsFindVisitor::ALL_CUTS can be used.
   * @return Struct with all found cuts.
   */
  CutStorage findCuts(GNet *net, unsigned int cutSize,
                      unsigned int maxCutsNumber = CutsFindVisitor::ALL_CUTS);

} // namespace eda::gate::optimizer
