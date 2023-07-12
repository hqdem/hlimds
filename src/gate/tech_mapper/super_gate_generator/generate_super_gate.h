//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "kitty/kitty.hpp"
#include "nlohmann/json.hpp"

#include <list>
#include <unordered_map>

#include <gate/tech_mapper/library/cell.h>
#include <gate/tech_mapper/super_gate_generator/node.h>

namespace eda::gate::techMap {

  class CircuitsGenerator {
public:
    void setLibElementsList(std::vector<Cell*> &newLibElements) {
      libElements = newLibElements;
    }

    std::vector<Node*> getGeneratedNodes() const { return nodesPrePrevLevel; }

    void initCircuit(int maxInputs);
    void generateCircuits();
private:
    bool getNextCombination(std::vector<Node*> &nextCombination,
      std::vector<unsigned> *indices, int *currIndex, std::size_t maxElementNumber);
    bool getPermutation(std::vector<unsigned> &indices, int *currIndex, std::size_t maxElementNumber);
    std::vector<Node*> tempNodesStorage;

    // Input data
    std::vector<Cell*> libElements;

    // Cells params
    unsigned maxNodesInCell;
    unsigned maxCellsArea;
    unsigned maxCellsDelay;
    unsigned maxCellsLevel;

    // Generators params
    unsigned maxGeneratedLevel;
    unsigned numTreads;

    // Nodes-predecessors for the given node.
    // Current level for the given node.
    std::vector<Node*> nodesCurrLevel;
    // Previous level for the given node.
    std::vector<Node*> nodesPrevLevel;
    // All the rest predecessors.
    std::vector<Node*> nodesPrePrevLevel;

    void createNode(Node *startNode, const std::vector<Node*> &nodes,
        Cell &element/*, unsigned long depth*/);
  };

} // namespace eda::gate::techMap
