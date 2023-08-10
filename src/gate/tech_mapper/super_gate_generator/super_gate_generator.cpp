//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/super_gate_generator/super_gate_generator.h"
#include "gate/model/gnet.h"

#include <kitty/kitty.hpp>
#include <omp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>
#include <vector>

namespace eda::gate::techMap {
using GNet = eda::gate::model::GNet;
using GateId = eda::gate::model::GNet::GateId;
using GateSymbol = eda::gate::model::GateSymbol;

void CircuitsGenerator::createNode(Node *startNode,
                                   const std::vector<Node*> &combination,
                                   Cell &libElement/*, unsigned long depth*/) {
  std::vector<Node*> inputNodes = combination;
  inputNodes.insert(inputNodes.begin(), startNode);

  // TODO add all the involved nodes in the combination
  //        to the list of the involved nodes of the start node
  //        if they have not been placed there before
  std::vector<Node*> involvedNodes = startNode->getInvolvedNodes();

  for (const auto &node : combination) {
    for (const auto &involved : node->getInvolvedNodes()) {
      if (std::find(involvedNodes.begin(), involvedNodes.end(), involved) !=
            involvedNodes.end()) {
        break;
      } else {
        involvedNodes.push_back(involved);
      }
    }
  }

  Cell *cell = new Cell(libElement);

  kitty::dynamic_truth_table *func =
      new kitty::dynamic_truth_table((startNode->getFunc())->num_vars());


  for (unsigned long i = 0; i < func->num_bits(); i++) {
    unsigned index = kitty::get_bit(*(startNode->getFunc()), i);
    unsigned pos = 1;
    for (const auto &node : combination) {
      index += kitty::get_bit(*(node->getFunc()), i) * pow(2, pos);
      pos++;
    }

    if (kitty::get_bit(*(libElement.getTruthTable()), index)) {
      kitty::set_bit(*func, i);
    }
  }

  Node *newNode = new Node(cell, 0);
  newNode->setFunc(func);

  newNode->addInputs(inputNodes);
  newNode->addInvolvedNodes(involvedNodes);
  newNode->addInvolvedNode(newNode);
  if (newNode->getInputs().size() != 0) {
      newNode->delaysCalculation();
  } else {
    newNode->setMaxDelay(0);
  }

  std::vector<Node*> combinedNodes; // TODO
  combinedNodes.reserve(nodesPrePrevLevel.size() + nodesPrevLevel.size() + nodesCurrLevel.size());
  combinedNodes.insert(combinedNodes.end(), nodesPrePrevLevel.begin(), nodesPrePrevLevel.end());
  combinedNodes.insert(combinedNodes.end(), nodesPrevLevel.begin(), nodesPrevLevel.end());
  combinedNodes.insert(combinedNodes.end(), nodesCurrLevel.begin(), nodesCurrLevel.end());

  bool truthTableExist = false;
  bool newNodeIsBetter = true;
  for (const auto &node : combinedNodes) {
    if (node->getCell()->getTruthTable() == newNode->getCell()->getTruthTable()) { // TODO how does this operator work?
      truthTableExist = true;
      if (node->getMaxDelay() > newNode->getMaxDelay()) {
        //newNodeIsBetter = true;
      }
    }
  }

  if (!truthTableExist || (truthTableExist && newNodeIsBetter)) {
    if (newNode->getInputs().size() != 0) {
      nodesCurrLevel.push_back(newNode);
    }
  }
}

void CircuitsGenerator::generateCombinations(
    Node *prevLayerNode,
    Cell *libElement,
    std::vector<std::vector<Node*>::iterator> &inputsIt) {
  long unsigned int next = 0;
  while (next != inputsIt.size()) {
    next = 0;
    std::vector<Node*> combination;
    for (long unsigned int i = 0; i < inputsIt.size(); i++) {
      std::vector<Node*>::iterator it = inputsIt.at(i);
      std::cout << "прошло" << std::endl;
      Node* nodePtr = *it;
      std::cout << inputsIt.size() << std::endl;
      //std::cout << (*(inputsIt.at(i)))->getCell()->getName() << std::endl;
      std::cout << "до" << std::endl;
      combination.push_back(nodePtr);
      //combination.push_back(*(inputsIt.at(i)));
      std::cout << "после" << std::endl;
    }
    createNode(prevLayerNode, combination, *libElement);
    for (long unsigned int i = 0; i <= inputsIt.size(); i++) {
      if (inputsIt[i] == nodesPrePrevLevel.end()) {
        next++;
        continue;
      } else {
        ++inputsIt[i];
        break;
      }
    }
  }
}

void CircuitsGenerator::generateCircuits() {
  long unsigned int depth = 0;
  while (depth != maxGeneratedLevel) {
    std::vector<Node*>::iterator prevLayerNode = nodesPrevLevel.begin();
    std::vector<Cell*>::iterator libElement = libElements.begin();

    do {
      std::vector<std::vector<Node*>::iterator> inputsIt;

      for (long unsigned int i = 0; i < 
          (*libElement)->getInputPinsNumber() - 1; ++i) {
        inputsIt.push_back(nodesPrePrevLevel.begin());
      }

      generateCombinations(*prevLayerNode, *libElement, inputsIt);

      if (prevLayerNode != nodesPrevLevel.end()) {
        ++prevLayerNode;
      } else {
        prevLayerNode = nodesPrevLevel.begin();
        ++libElement;
      }
    }
    while ((prevLayerNode != nodesPrevLevel.end()) &
        (libElement != libElements.end()));

    nodesPrePrevLevel.insert(nodesPrePrevLevel.end(),
                             nodesPrevLevel.begin(), nodesPrevLevel.end());
    nodesPrevLevel = nodesCurrLevel;
    nodesCurrLevel.clear();
  }
}

void CircuitsGenerator::initCircuit(int inputsNumber) {
  std::vector<std::string> inputNames;
  std::vector<Pin> inputPins;

  for (int i = 1; i <= inputsNumber; i++) {
    const std::string formula =
     (const std::string)(i == 1 ? "A" : i == 2 ? "B" : std::to_string(i - 1));

    inputNames.push_back(formula);
    inputPins.push_back(Pin(formula, 0, 0, 0, 0)); // TODO
  }

  for (int i = 1; i <= inputsNumber; i++) {
    kitty::dynamic_truth_table *truthTable =
      new kitty::dynamic_truth_table(inputNames.size());

    const std::string formula =
     (const std::string)(i == 1 ? "A" : i == 2 ? "B" : std::to_string(i - 1));
    kitty::create_from_formula(*truthTable, formula, inputNames);

    Cell *cell = new Cell(formula, inputPins, truthTable);
    Node *node = new Node(cell, 1); // TODO: max delay is 1
    node->setFunc(truthTable);
    node->addInvolvedNode(node); // TODO: for some reasons node is involved
    nodesPrevLevel.push_back(node);
  }
}


void CircuitsGenerator::translateNodeIntoGnet() {
  for (const auto &node : nodesPrePrevLevel) {
    GNet net;
    std::unordered_map<Node*, GateId> transferMap;
    GNet::SignalList inputs;
    for (const auto &InvolvedNode : node->getInvolvedNodes()) {
      GateId gateId = net.addGate(GateSymbol::create("random"),inputs);
      transferMap.emplace(InvolvedNode, gateId);
      //TODO
    }
  }
}

} // namespace eda::gate::techMap
