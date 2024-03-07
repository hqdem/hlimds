//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "npndb.h"
#include "util/kitty_utils.h"

namespace eda::gate::optimizer {

NPNDatabase::TT NPNDatabase::applyGateFunc(const model::GateSymbol::Value func,
                                           const std::vector<TT> &inputList) {
  TT result;
  switch (func) {
  case GateSymbol::ZERO:
    result = TT(inputList[0].num_vars());
    break;
  case GateSymbol::ONE:
    result = TT(inputList[0].num_vars());
    for (auto &block : result) {
      block = ~(uint64_t)(0);
    }
    break;
  case GateSymbol::IN:
  case GateSymbol::NOP:
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

NPNDatabase::TT NPNDatabase::buildTT(const BoundGNet &bGNet) {
  size_t inputCount = bGNet.inputBindings.size(),
         outputCount = bGNet.outputBindings.size();
  assert(outputCount == 1);
  TT result;
  std::unordered_map<Gate::Id, uint32_t> rInputs;
  for (size_t i = 0; i < inputCount; i++) {
    rInputs[bGNet.inputBindings[i]] = i;
  }
  std::unordered_map<Gate::Id, TT> ttMap;
  for (auto *gate : bGNet.net->gates()) {
    Gate::Id gateId = gate->id();
    TT curResult(inputCount);
    if (gate->isSource()) {
      assert(rInputs.find(gateId) != rInputs.end());
      kitty::create_nth_var(curResult, rInputs[gateId]);
    } else {
      std::vector<TT> inputList;
      for (auto signal : gate->inputs()) {
        Gate::Id inputId = signal.node();
        TT inputTT = ttMap[inputId];
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

NPNDatabase::NPNTransformation NPNDatabase::inverse(const NPNTransformation &t) {
  uint32_t negationMask = 0;
  InputPermutation permutation = InputPermutation(t.permutation.size());
  for (size_t i = 0; i < permutation.size(); i++) {
    permutation[t.permutation[i]] = i;
    if (t.negationMask & (1 << t.permutation[i])) {
      negationMask = negationMask | (1 << i);
    }
  }
  negationMask = negationMask | (t.negationMask & (1 << permutation.size()));
  return NPNTransformation{negationMask, permutation};
}

void NPNDatabase::npnTransformInplace(BoundGNet &bGNet,
                                      const NPNTransformation &t) {
  uint16_t negationMask = t.negationMask;
  InputPermutation permutation = t.permutation;
  size_t inputCount = bGNet.inputBindings.size();
  assert(permutation.size() == inputCount && "invalid permutation");
  assert(bGNet.outputBindings.size() == 1 && "too many outputs");
  for (size_t i = 0; i < inputCount; i++) {
    if (((negationMask >> i) & 1) == 1) {
      const Gate::Id inputId = bGNet.inputBindings[i];
      const Gate::Id newInputId = bGNet.net->addIn();
      const Gate::Id notGateId = bGNet.net->addNot(newInputId);
      bGNet.net->replace(inputId, notGateId);
      bGNet.inputBindings[i] = newInputId;
      bGNet.net->setGate(notGateId, GateSymbol::NOT,
                         Gate::SignalList{Gate::Signal::always(newInputId)});
      bGNet.net->sortTopologically();
    }
  }
  if (((negationMask >> inputCount) & 1) == 1) {
    const Gate::Id outId = bGNet.outputBindings[0];
    const Gate::Id preOutId = Gate::get(outId)->inputs()[0].node();
    const Gate::Id notGateId = bGNet.net->addNot(preOutId);
    const Gate::Id newOutId = bGNet.net->addOut(notGateId);
    bGNet.net->removeGate(outId);
    bGNet.net->sortTopologically();
    bGNet.outputBindings[0] = newOutId;
  }
  const BoundGNet::GateBindings oldInputBindings = bGNet.inputBindings;
  for (size_t i = 0; i < inputCount; i++) {
    assert(permutation[i] < inputCount && "invalid permutation");
    bGNet.inputBindings[i] = oldInputBindings[permutation[i]];
  }
}

BoundGNet NPNDatabase::npnTransform(const BoundGNet &bGNet,
                                    const NPNTransformation &t) {
  BoundGNet result = bGNet.clone();
  npnTransformInplace(result, t);
  return result;
}

NPNDatabase::GetResult NPNDatabase::get(const TT &tt) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  return std::make_tuple(storage[utils::getTT(config)], inverse(t));
}

NPNDatabase::GetResult NPNDatabase::get(const BoundGNet &bnet) {
  TT tt = buildTT(bnet);
  return get(tt);
}

NPNDatabase::NPNTransformation NPNDatabase::push(const BoundGNet &bnet) {
  BoundGNet bnetClone = bnet.clone();
  TT tt = buildTT(bnetClone);
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  npnTransformInplace(bnetClone, t);
  storage[std::get<0>(config)].push_back(bnetClone);
  return t;
}

void NPNDatabase::erase(const TT &tt) {
  storage[tt] = {};
}

} // namespace eda::gate::optimizer
