//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "truthtable.h"

namespace eda::gate::optimizer {

TruthTable TruthTable::buildNthVar(int n) {
  uint64_t rawResult = 0;
  int div = (1 << n);
  for (uint64_t i = 0; i < 64; i++) {
    if ((i / div) % 2 == 1) {
      rawResult = rawResult | (1ull << i);
    }
  }
  return TruthTable(rawResult);
}

TruthTable TruthTable::applyGateFunc(const GateSymbol::Value func,
                                     const TruthTableList &inputList) {
  TruthTable result;
  switch (func) {
  case GateSymbol::ZERO:
    result = TruthTable::zero();
    break;
  case GateSymbol::ONE:
    result = TruthTable::one();
    break;
  case GateSymbol::NOP:
    assert(inputList.size() == 1);
    result = inputList[0];
    break;
  case GateSymbol::IN:
    assert(inputList.size() == 1);
    result = inputList[0];
    break;
  case GateSymbol::OUT:
    assert(inputList.size() == 1);
    result = inputList[0];
    break;
  case GateSymbol::NOT:
    assert(inputList.size() == 1);
    result = inputList[0];
    result = ~result;
    break;
  case GateSymbol::AND:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result & inputList[i];
    }
    break;
  case GateSymbol::OR:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result | inputList[i];
    }
    break;
  case GateSymbol::XOR:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result ^ inputList[i];
    }
    break;
  case GateSymbol::NAND:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result & inputList[i];
    }
    result = ~result;
    break;
  case GateSymbol::NOR:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result | inputList[i];
    }
    result = ~result;
    break;
  case GateSymbol::XNOR:
    result = inputList[0];
    for (size_t i = 1; i < inputList.size(); i++) {
      result = result ^ inputList[i];
    }
    result = ~result;
    break;
  case GateSymbol::MAJ:
    if (inputList.size() == 3) {
      result = (inputList[0] & inputList[1]) |
               (inputList[0] & inputList[2]) |
               (inputList[1] & inputList[2]);
    }
    break;
  default:
    assert(false && "Unsupported gate");
    break;
  }
  return result;
}

TruthTable TruthTable::build(const BoundGNet &bGNet) {
  TruthTable result;
  if (!bGNet.net->isSorted()) {
    bGNet.net->sortTopologically();
  }
  std::unordered_map<Gate::Id, uint32_t> rInputs;
  for (size_t i = 0; i < bGNet.inputBindings.size(); i++) {
    rInputs[bGNet.inputBindings[i]] = i;
  }

  std::unordered_map<Gate::Id, TruthTable> ttMap;
  for (auto *gate : bGNet.net->gates()) {
    Gate::Id gateId = gate->id();

    TruthTable curResult;
    if (gate->isSource()) {
      assert(rInputs.find(gateId) != rInputs.end());
      curResult = buildNthVar(rInputs[gateId]);
    } else {
      TruthTableList inputList;
      for (auto signal : gate->inputs()) {
        Gate::Id inputId = signal.node();
        TruthTable inputTT = ttMap[inputId];
        inputList.push_back(inputTT);
      }
      curResult = applyGateFunc(gate->func(), inputList);
    }

    if (gate->isTarget()) {
      result = curResult;
    }

    ttMap[gateId] = curResult;
  }

  return result;
}

} // namespace eda::gate::optimizer
