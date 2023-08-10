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

namespace eda::gate::techMap {

struct Node {
  Node(Cell *cell, double maxDelay) :
    maxDelay(maxDelay), cell(cell) {}

  void addInvolvedNode(Node* node) {involvedNodes.push_back(node);}
  const std::vector<Node*> &getInvolvedNodes() { return involvedNodes; }
  void addInvolvedNodes(std::vector<Node*> nodes) {
    involvedNodes.insert(involvedNodes.end(), nodes.begin(), nodes.end());
  }
  
  void addInput(Node* node) {inputs.push_back(node);}
  const std::vector<Node*> &getInputs() { return inputs; }
  void addInputs(std::vector<Node*> nodes) {
    inputs.insert(inputs.end(), nodes.begin(), nodes.end());
  }

  Cell* getCell() { return cell; }

  void setMaxDelay(double delay) { maxDelay = delay; }
  double getMaxDelay() { return maxDelay; }

  void setFunc(kitty::dynamic_truth_table *func) { this->func = func; }
  kitty::dynamic_truth_table *getFunc() { return func; }

  void delaysCalculation();

  const std::vector<double> getDelays() { return delays; }

private:
  std::vector<Node*> inputs;
  std::vector<Node*> involvedNodes;
  std::vector<double> delays;
  double maxDelay;
  Cell *cell;
  kitty::dynamic_truth_table *func;
  };
  } // namespace eda::gate::techMap
