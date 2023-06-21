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

namespace eda::gate::optimizer {

  //using Cell = eda::gate::library::Cell;

  struct Node {
    Node(Cell *cell, double maxDelay) :
      maxDelay(maxDelay), cell(cell) {}

    const std::vector<Node*> &getInvolvedNodes() { return involvedNodes; }
    void addInvolvedNode(Node* node) {
      involvedNodes.push_back(node);
    }
    void addInvolvedNodes(std::vector<Node*> nodes) {
      involvedNodes.insert(involvedNodes.end(), nodes.begin(), nodes.end());
    }

    const std::vector<Node*> &getInputs() { return inputs; }
    void addInput(Node* node) {
      inputs.push_back(node);
    }
    void addInputs(std::vector<Node*> nodes) {
      inputs.insert(inputs.end(), nodes.begin(), nodes.end());
    }

    Cell* getCell() { return cell; }

    void setMaxDelay(double delay) { maxDelay = delay; }
    double getMaxDelay() { return maxDelay; }

    void setFunc(kitty::dynamic_truth_table *func) { func = func; }
    kitty::dynamic_truth_table *getFunc() { return func; }

    void delaysCalculation();
    const std::vector<double> getDelays() { return delays; }

private:
    std::vector<Node*> inputs; // todo: why inputs are also Node* ?
    std::vector<Node*> involvedNodes;
    std::vector<double> delays;
    double maxDelay;
    Cell *cell;
    kitty::dynamic_truth_table *func;
  };

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

} // namespace eda::gate::optimizer
