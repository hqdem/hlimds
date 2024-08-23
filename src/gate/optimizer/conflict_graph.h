//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <map>
#include <set>

namespace eda::gate::optimizer { 
/// @cond ALIASES
using Subnet = eda::gate::model::Subnet;
using SubnetID = eda::gate::model::SubnetID;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
/// @endcond

/**
 * @brief Finding the set of disjoint subcircuits of 
 * maximum weight for the future replacement.
 * 
 * The task is presented in the form of a graph.
 * The vertex of the graph is the subcircuit of initial circuit.
 * An edge means that there is an intersection between this subcircuits.
 * The weigth of vertex is the effect of the replacement.
 */
class ConflictGraph {
public:

  /// Empty constructor.
  ConflictGraph() {};

  /**
   * @brief Finds the set of disjoint vertexes with max total weight.
   * A greedy algorithm is used.
   *
   * @param builder SubnetBuilder with the initial circuit.
   */
  float findBestColoring(SubnetBuilder *builder);

  /**
   * @brief Adds a vertex(subcircuit) to the graph.
   *
   * @param delta effect of the replacement current subcircuit.
   * @param subnetID new subcircuit for future replacement.
   * @param mapping mapping for future replacement.
   * @param stack stack with numbers of cells that are present in the sybcircuit 
   */
  void addVertice(float delta, 
                  SubnetID subnetID, 
                  model::InOutMapping &mapping, 
                  std::vector<size_t> &numCells) {
    
    Vertice *v = new Vertice;
    v->delta = delta; 
    v->numCells = numCells, 
    v->entryMap = mapping, 
    v->subnetID = subnetID;
    graph.push_back(v);
  }

private:

  /// Struct for describing the vertices of a graph.
  struct Vertice {
    float delta;
    std::vector<size_t> numCells;
    model::InOutMapping entryMap;
    SubnetID subnetID;
  };
  
  std::vector<bool> isVisited;

  /// Graph representation 
  std::vector<Vertice*> graph;

  float coloring(SubnetBuilder *builder);
};

} // namespace eda::gate::optimizer 
