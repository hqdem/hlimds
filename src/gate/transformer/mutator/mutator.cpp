//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/transformer/mutator/mutator.h"

namespace eda::gate::mutator {
  
  using Gate = eda::gate::model::Gate;
  using GateSymbol = eda::gate::model::GateSymbol;
  using eda::gate::optimizer::getNext;
  using Signal = eda::gate::model::Gate::Signal;
  using SignalList = eda::gate::model::Gate::SignalList;
  using Walker = eda::gate::optimizer::Walker;

  //===--------------------------------------------------------------------===//
  // Supporting functions
  //===--------------------------------------------------------------------===//
  GateIdList makeListReplacedGates(const GNet &inputGNet, 
                                   GateIdList &listGates) {
    size_t gateNum = inputGNet.nGates();
    GateIdList replacedGates;
    GateId firstGateId = inputGNet.gates()[0]->id();
    if (listGates.empty()) {
      for (auto *gate : inputGNet.gates()) {
        replacedGates.push_back(gate->id() + gateNum);
      }
    } else {
      for (GateId gateId : listGates) {
        GateId id = (gateId >= firstGateId) ? 0 : firstGateId;
        replacedGates.push_back(gateId + gateNum + id);
      }
    }
    return replacedGates;
  }

  MutatorVisitor runVisitor(GNet &inputGNet, 
                            int numberOfGates,
                            GateIdList &listGates,
                            GateSymbolList function) {
    GNet mutatorGNet = *inputGNet.clone();
    GateIdList replacedGates = makeListReplacedGates(inputGNet, listGates);
    MutatorVisitor mutatorVisitor(mutatorGNet, numberOfGates, replacedGates, function);
    Walker gNetWalker(&mutatorGNet, &mutatorVisitor);
    gNetWalker.walk(true);
    return mutatorVisitor;
  }
  
  //===--------------------------------------------------------------------===//
  // Static functions
  //===--------------------------------------------------------------------===//  
  GNet Mutator::mutate(GNet &inputGNet, 
                       GateIdList &listGates,
                       GateSymbolList function) {
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, listGates.size(), listGates, function);
    return mutatorVisitor.getGNet();
  }

  GNet Mutator::mutate(int &counter,
                       GNet &inputGNet,
                       int numberOfGates,
                       GateSymbolList function) {
    GateIdList nullList = {};
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, numberOfGates, nullList, function);
    counter = mutatorVisitor.getNumChangedGates();
    return mutatorVisitor.getGNet();
  }

  GNet Mutator::mutate(GNet &inputGNet,
                       int numberOfGates,
                       GateSymbolList function) {
    GateIdList nullList = {};
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, numberOfGates, nullList, function);
    return mutatorVisitor.getGNet();
  }
} //namespace eda::gate::mutator
