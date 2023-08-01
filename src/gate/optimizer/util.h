//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/bgnet.h"
#include "gate/optimizer/cone_visitor.h"
#include "gate/optimizer/links_clean_counter.h"
#include "gate/optimizer/walker.h"

/**
 * \brief Methods for circuit model optimization.
 */
namespace eda::gate::optimizer {

  using Gate = model::Gate;
  using GateId = model::GNet::GateId;
  using GNet = model::GNet;
  using ConeSet = std::unordered_set<GateId>;
  using Cut = CutStorage::Cut;
  using Order = std::vector<GateId>;

  //===--------------------------------------------------------------------===//
  // Model modification methods
  //===--------------------------------------------------------------------===//

  /**
   * \brief Returns neighbours of the node.
   * @param node The target node.
   * @param forward The direction of neighbours (parent or child).
   * @return List of node predecessors or successors depending on forward flag.
   */
  std::vector<GNet::GateId> getNext(GateId node, bool forward);

  /**
   * \brief Removes the start node and others that were used only by the start.
   * @param net Net to delete nodes from.
   * @param start Node to start recursive deleting with.
   */
  void rmRecursive(GNet *net, GateId start);

  //===--------------------------------------------------------------------===//
  // Cut-related methods
  //===--------------------------------------------------------------------===//

  /**
   * \brief Checks that a given cut is indeed a cut for a given vertex.
   */
  bool isCut(const GateId &gate, const Cut &cut, GateId &failed);

  //===--------------------------------------------------------------------===//
  // Cone-related methods
  //===--------------------------------------------------------------------===//

  /**
   * \brief Finds all nodes that are part of a maximum cone for the node.
   * @param start Vertex of the cone.
   * @param cone Set of nodes, that make up the cone will be stored.
   * @param forward Direction of building a cone.
   */
  void getConeSet(GateId start, ConeSet &cone, bool forward);

  /**
   * \brief Finds all nodes that are part of a cone for the node.
   * @param start Vertex of the cone.
   * @param cut Nodes that restricting the base of a cone.
   * @param coneNodes Set of nodes, that make up the cone will be stored.
   * @param forward Direction of building a cone.
   */
  void getConeSet(GateId start, const Cut &cut, ConeSet &cone, bool forward);

  /**
   * \brief Cone extraction function.
   * @param net Net where cone extraction is executed.
   * @param root Vertex for which the cone is constructed.
   * @param cut Cut that forms the cone.
   * @param order The order will be kept when constructing correspondence map.
   * @return Extracted cone with input correspondence map.
   */
  BoundGNet extractCone(const GNet *net,
                        GateId root,
                        Cut &cut,
                        const Order &order);

  /**
   * \brief Cone extraction function.
   * @param net Net where cone extraction is executed.
   * @param root Vertex for which the cone is constructed.
   * @param order The order will be kept when constructing correspondence map.
   * @return Extracted cone with input correspondence map.
   */
  BoundGNet extractCone(const GNet *net, GateId root, const Order &order);
} // namespace eda::gate::optimizer

