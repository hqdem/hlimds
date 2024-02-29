//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/transformer/mutator/mutator.h"
#include "util/logging.h"

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
    GateIdList makeListGate(GNet &inputGNet,
                            unsigned int numOfCuts,
                            GateIdList &listGates,
                            unsigned int cutSize) {
    if (inputGNet.isEmpty()) {
      LOG_WARN << "Input GNet is empty\n";
      return {};
    }
    CutStorage cutStorage = findCuts(&inputGNet, cutSize);
    GateIdList answerList;
    GateId firstGateId = inputGNet.gates()[0]->id();
    if (!listGates.empty()) {
      for (unsigned int i = 0; i < listGates.size(); i++) {
        listGates[i] = (listGates[i] < firstGateId)? 
                        firstGateId + listGates[i] :
                        listGates[i];
      }
    } else {
      unsigned int count = 0;
      const auto &gate = inputGNet.gates();
      GateId gateId = gate[inputGNet.nGates() - 1]->id();
      GateId firstGate = gate[0]->id();
      while (count < numOfCuts && gateId >= firstGate) {
        if (!Gate::get(gateId)->isTarget() && !Gate::get(gateId)->isSource()) {
          listGates.push_back(gateId);
          count++;
        }
        gateId--;
      }
    }
    for (GateId gateId : listGates) {
      if (!Gate::get(gateId)->isTarget() && !Gate::get(gateId)->isSource()) {
        auto findGateId = std::find(answerList.begin(), 
                                    answerList.end(), 
                                    gateId);
        if (findGateId == answerList.end()) {
          answerList.push_back(gateId);
        }
        auto &cuts = cutStorage.cuts[gateId];
        for (const auto &cut : cuts) {
          for (GateId cutGateId : cut) {
            auto findGate = std::find(answerList.begin(), 
                                      answerList.end(), 
                                      cutGateId);
            if (findGate == answerList.end()) {
              answerList.push_back(cutGateId);
            }
          }
        }
      }
    }
    return answerList;
  }
  
  GateIdList makeListReplacedGates(const GNet &inputGNet, 
                                   GateIdList &listGates) {
    GateIdList replacedGates;
    GateId firstGateId = inputGNet.gates()[0]->id();
    if (listGates.empty()) {
      for (auto *gate : inputGNet.gates()) {
        replacedGates.push_back(gate->id());
      }
    } else {
      for (GateId gateId : listGates) {
        if (gateId < inputGNet.nGates() || gateId >= firstGateId) {
          GateId id = (gateId >= firstGateId) ? 0 : firstGateId;
          replacedGates.push_back(gateId + id);
        } else if (gateId >= firstGateId - inputGNet.nGates() &&
                   gateId < firstGateId) {
          replacedGates.push_back(gateId + inputGNet.nGates());
        } else {
          LOG_WARN << "Wrong gate id : " << gateId << std::endl;
        }
      }
    }
    return replacedGates;
  }

  int paramForVisitor(MutatorMode mode,
                      GNet &inputGNet, 
                      unsigned int &number,
                      GateIdList &gatesList,
                      unsigned int &cutSize) {
    if (mode == MutatorMode::CUT) {
      gatesList = makeListGate(inputGNet, number, gatesList, cutSize);
      number = gatesList.size();
    } else if (mode != MutatorMode::GATE) {
      LOG_WARN << "Unexpected flag : " << mode;
      return 0;
    }
    return 1;
  }

  MutatorVisitor runVisitor(GNet &inputGNet, 
                            int numberOfGates,
                            GateIdList &listGates,
                            GateSymbolList function) {
    GNet mutatorGNet = *inputGNet.clone();
    GateIdList replacedGates = makeListReplacedGates(mutatorGNet, listGates);
    MutatorVisitor mutatorVisitor(mutatorGNet, 
                                  numberOfGates, 
                                  replacedGates, 
                                  function); 
    Walker gNetWalker(&mutatorGNet, &mutatorVisitor);
    gNetWalker.walk(true);
    return mutatorVisitor;
  }
  
  //===------------------------------------------------------------------===//
  // Static functions
  //===------------------------------------------------------------------===//  
  GNet Mutator::mutate(MutatorMode mode,
                       GNet &inputGNet, 
                       GateIdList &listGates,
                       GateSymbolList function,
                       unsigned int cutSize) {
    GateIdList gatesList;
    if (mode == MutatorMode::CUT) {
      gatesList = makeListGate(inputGNet, listGates.size(), listGates, cutSize);
    } else if (mode == MutatorMode::GATE) {
      gatesList = listGates;
    } else {
      LOG_WARN << "Unexpected flag : " << mode;
      return inputGNet;
    }
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, gatesList.size(), 
                                               gatesList, function);
    listGates = mutatorVisitor.listMutatedGate();
    return mutatorVisitor.getGNet();
  }

  GNet Mutator::mutate(MutatorMode mode,
                       int &counter,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function,
                       unsigned int cutSize) {
    GateIdList gatesList = {};
    unsigned int number = num;
    paramForVisitor(mode, inputGNet, number, gatesList, cutSize);
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, number, 
                                               gatesList, function);
    counter = mutatorVisitor.getNumChangedGates();
    return mutatorVisitor.getGNet();
  }

  GNet Mutator::mutate(MutatorMode mode,
                       GateIdList &mutatedGates,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function,
                       unsigned int cutSize) {
    GateIdList gatesList = {};
    unsigned int number = num;
    paramForVisitor(mode, inputGNet, number, gatesList, cutSize);
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, number, 
                                               gatesList, function);
    mutatedGates = mutatorVisitor.listMutatedGate();
    return mutatorVisitor.getGNet();
  }

  GNet Mutator::mutate(MutatorMode mode,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function,
                       unsigned int cutSize) {
    GateIdList gatesList = {};
    unsigned int number = num;
    paramForVisitor(mode, inputGNet, number, gatesList, cutSize);
    MutatorVisitor mutatorVisitor = runVisitor(inputGNet, number, 
                                               gatesList, function);
    return mutatorVisitor.getGNet();
  }
} //namespace eda::gate::mutator
