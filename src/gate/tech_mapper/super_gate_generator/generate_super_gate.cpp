//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/super_gate_generator/generate_super_gate.h"

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

bool CircuitsGenerator::getPermutation(std::vector<unsigned> &indices,
       int *currIndex, std::size_t maxElementNumber) {
  do {
    if (indices[*currIndex] == maxElementNumber - 1) {
      --(*currIndex);
      if (*currIndex < 0) {
        return false; } }
    else {
      ++(indices[*currIndex]);
      while ((std::size_t)*currIndex < indices.size() - 1) {
        indices[*currIndex + 1] = 0;
        ++(*currIndex);
      }
      return true;
    }
  } while (true);
}

// TODO we need a temporal storage of layersLE2 + lastNodes vectors of Node* type
bool CircuitsGenerator::getNextCombination(
    std::vector<Node*> &nextCombination,
    std::vector<unsigned> *indices,
    int *currIndex, std::size_t maxElementNumber) {
  bool hasNext = getPermutation(*indices, currIndex, maxElementNumber);
  nextCombination.clear();
  std::cout << "indices (max= " << maxElementNumber << ", currIndex=" << *currIndex << "): ";
  for(const auto index : *indices) {
    nextCombination.push_back(tempNodesStorage[index]);
    std::cout << index << " ";
  }
  std::cout << std::endl;
  return hasNext;
}

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

void CircuitsGenerator::generateCircuits() {
  int depth = 0;
  while (depth != maxGeneratedLevel) {
    std::vector<Node*>::iterator prevLayerNode = nodesPrevLevel.begin();
    std::vector<Cell*>::iterator libElement = libElements.begin();

    while ((prevLayerNode != nodesPrevLevel.end()) &
        (libElement != libElements.end())) {
      int inputsIt[(*libElement)->getInputPinsNumber()];

      std::vector<Node*> combination;
      for (int i = 0; i < (*libElement)->getInputPinsNumber(); i++) {
        combination.push_back(nodesPrePrevLevel.at(inputsIt[i]));
      }
      createNode(*prevLayerNode, combination, *libElement);

      for (int i = 0; i < (*libElement)->getInputPinsNumber(); i++) {
        if (inputsIt[i] = nodesPrePrevLevel)
        combination.push_back(nodesPrePrevLevel.at(inputsIt[i]));
      }
    }
  }



  for(unsigned depth = 0; depth < 1; depth++) {
    for (const auto& prevLayerNode : nodesPrevLevel) {
      for (const auto& libElement : libElements) {
        std::cout << libElement->getInputPinsNumber() << std::endl;
        switch(libElement->getInputPinsNumber()) {
          case 1:
            createNode(prevLayerNode, {}, *libElement);
            break;

          case 2:
            for (const auto& firstNode : nodesPrePrevLevel) {
              std::vector<Node*> combination = {firstNode};
              createNode(prevLayerNode, combination, *libElement);
            }
            break;

          case 3:
            for (const auto& firstNode : nodesPrePrevLevel) {
              for (const auto& secondNode : nodesPrePrevLevel) {
                std::vector<Node*> combination = {firstNode, secondNode};
                createNode(prevLayerNode, combination, *libElement);
              }
            }
            break;

          case 4:
            for (const auto& firstNode : nodesPrePrevLevel) {
              for (const auto& secondNode : nodesPrePrevLevel) {
                for (const auto& thirdNode : nodesPrePrevLevel) {
                  std::vector<Node*> combination = {firstNode, secondNode, thirdNode};
                  createNode(prevLayerNode, combination, *libElement);
                }
              }
            }
            break;

          case 5:
            for (const auto& firstNode : nodesPrePrevLevel) {
              for (const auto& secondNode : nodesPrePrevLevel) {
                for (const auto& thirdNode : nodesPrePrevLevel) {
                  for (const auto& fourthdNode : nodesPrePrevLevel) {
                    std::vector<Node*> combination = {firstNode, secondNode, thirdNode, fourthdNode};
                    createNode(prevLayerNode, combination, *libElement);
                  }
                }
              }
            }
            break;

          case 6:
            for (const auto& firstNode : nodesPrePrevLevel) {
              for (const auto& secondNode : nodesPrePrevLevel) {
                for (const auto& thirdNode : nodesPrePrevLevel) {
                  for (const auto& fourthdNode : nodesPrePrevLevel) {
                    for (const auto& fifthNode : nodesPrePrevLevel) {
                      std::vector<Node*> combination = {firstNode, secondNode, thirdNode, fourthdNode, fifthNode};
                      createNode(prevLayerNode, combination, *libElement);
                    }
                  }
                }
              }
            }
            break;
        }
      }
    }
    nodesPrePrevLevel.insert(nodesPrePrevLevel.end(),
                             nodesPrevLevel.begin(), nodesPrevLevel.end());
    nodesPrevLevel = nodesCurrLevel;
    nodesCurrLevel.clear();
  }

  /*
  const uint MAX = 3; // TODO: the max number of element in permutation
  for(unsigned depth = 0; depth < 2; depth++) {
    tempNodesStorage.clear();
    tempNodesStorage.reserve(nodesPrePrevLevel.size() + nodesPrevLevel.size());
    tempNodesStorage.insert(tempNodesStorage.end(), nodesPrePrevLevel.begin(), nodesPrePrevLevel.end());
    tempNodesStorage.insert(tempNodesStorage.end(), nodesPrevLevel.begin(), nodesPrevLevel.end());
    std::vector<uint> *indices = new std::vector<uint>(MAX, 0); // TODO
    std::size_t maxElements = tempNodesStorage.size(); // TODO

    for (const auto& prevLayerNode : nodesPrevLevel) {
      for (const auto& libElement : libElements) {
        std::vector<Node*> combination;
        int currentIndex = MAX - 1; // TODO
        for(uint i = 0; i < indices->size(); i++) { // TODO
          (*indices)[i] = 0;
        }
        std::cout << "depth=" << depth << ", preNode=" << prevLayerNode->getCell()->getName() << ", libElement=" << libElement->getName() << "\n";
        while(getNextCombination(combination, indices, &currentIndex, maxElements)) {
          createNode(prevLayerNode, combination, *libElement/*, depth);
        }
      }
    }

    delete indices;
    nodesPrePrevLevel.insert(nodesPrePrevLevel.end(),
                             nodesPrevLevel.begin(), nodesPrevLevel.end());
    nodesPrevLevel = nodesCurrLevel;
    nodesCurrLevel.clear();
  }
  */
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

} // namespace eda::gate::techMap
