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
#include "gate/optimizer/walker.h"

/**
 * \brief Methods used for rewriting.
 */
namespace eda::gate::optimizer {

  using GNet = model::GNet;
  using GateID = model::GNet::GateId;
  using Gate = model::Gate;
  using Cut = CutStorage::Cut;

  /**
   * @param node
   * @param forward
   * @return List of node predecessors or successors depending on forward flag.
   */
  std::vector<GNet::GateId> getNext(GateID node, bool forward);

  /**
   * Finds all nodes that are part of a maximum cone for the node.
   * @param start Vertex of the cone.
   * @param coneNodes Set of nodes, that make up the cone will be stored.
   * @param forward Direction of building a cone.
   */
  void getConeSet(GateID start, std::unordered_set<GateID> &coneNodes, bool forward);

  /**
   * Finds all nodes that are part of a cone for the node.
   * @param start Vertex of the cone.
   * @param cut Nodes that restricting the base of a cone.
   * @param coneNodes Set of nodes, that make up the cone will be stored.
   * @param forward Direction of building a cone.
   */
  void getConeSet(GateID start, const Cut &cut, std::unordered_set<GateID> &coneNodes,  bool forward);

} // namespace eda::gate::optimizer

