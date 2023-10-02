//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/tech_optimizer/super_gate_generator/super_gate_generator.h"

using Link = eda::gate::model::Link;
using CellType = eda::gate::model::CellType;

namespace eda::gate::tech_optimizer {

SuperGateGenerator::SuperGateGenerator(std::vector<const CellTypeID*> &library, 
    unsigned int maxSuperGatesInputs, unsigned int maxDepth) {
      this->library = library;
      this->maxSuperGatesInputs = maxSuperGatesInputs;
      this->maxDepth = maxDepth;
    }

std::vector<Subnet*> SuperGateGenerator::generate() {
  unsigned int depth = 0;
  while (depth < maxDepth) {
    while (!outOfCombinations()) {
      createSuperGate();
      getNextComb();
    }
  }
}

void SuperGateGenerator::createSuperGate() {
  eda::gate::model::SubnetBuilder subnetBuilder;

  // Добавялем входы в Subnet
  unsigned int inputsID[maxSuperGatesInputs];
  for (unsigned int i = 0; i < maxSuperGatesInputs; ++i) {
    inputsID[i] = subnetBuilder.addCell(eda::gate::model::CellSymbol::IN);
  }

  unsigned int itInput = 0;

  // Подсоединяем входные ячейки ко входам
  for (const auto &inputElem : inputsElem) {
    // inputElem первым элементом содержит корневцю ячейку 
    if (inputElem == inputsElem.begin()) {continue;}

    unsigned int midCellID[inputsElem.size() - 1];
    Subnet::LinkList linkList;
    unsigned int i = 0;
    for (itInput; itInput < CellType::get(*(*inputElem)).getInNum() + itInput; ++itInput) {
      size_t sizeValue = static_cast<size_t>(inputsID[itInput]);
      Link link(sizeValue);
      linkList.push_back(link);
      midCellID[i] = subnetBuilder.addCell(*(*inputElem), linkList);
      i++;
    }
  }
}

bool SuperGateGenerator::outOfCombinations() {
  bool outOfCombination = true;
  for (auto &inputElem : inputsElem) {
    if (inputElem != --inputElem.end()) {
      outOfCombination = false;
    }
  }
  return outOfCombination;
}
} // namespace eda::gate::tech_optimizer
