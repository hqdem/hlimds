//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "kitty_utils.h"

namespace eda::utils {

using BoundGNet = gate::optimizer::BoundGNet;
using Gate = gate::model::Gate;
using GateSymbol = gate::model::GateSymbol;
using TT = kitty::dynamic_truth_table;

TT toTT(uint64_t x) {
  TT tt(6);
  *tt.begin() = x;
  return tt;
}

void npnTransformInplace(BoundGNet &bGNet,
                         const NPNTransformation &t) {
  uint16_t negationMask = t.negationMask;
  NPNTransformation::InputPermutation permutation = t.permutation;
  size_t inputCount = bGNet.inputBindings.size();
  assert(permutation.size() == inputCount && "invalid permutation");
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
    Gate::Id outId = 0;
    if (bGNet.outputBindings.empty()) {
      for (const auto& gate : bGNet.net->gates()) {
        if (gate->isTarget()) {
          outId = gate->id();
          break;
        }
      }
    } else {
      outId = bGNet.outputBindings[0];
    }
    const Gate::Id preOutId = Gate::get(outId)->inputs()[0].node();
    const Gate::Id notGateId = bGNet.net->addNot(preOutId);
    const Gate::Id newOutId = bGNet.net->addOut(notGateId);
    bGNet.net->removeGate(outId);
    bGNet.net->sortTopologically();
    if (bGNet.outputBindings.empty()) {
      bGNet.outputBindings.resize(1);
    }
    bGNet.outputBindings[0] = newOutId;
  }
  const BoundGNet::GateBindings oldInputBindings = bGNet.inputBindings;
  for (size_t i = 0; i < inputCount; i++) {
    assert(permutation[i] < inputCount && "invalid permutation");
    bGNet.inputBindings[i] = oldInputBindings[permutation[i]];
  }
}

BoundGNet npnTransform(const BoundGNet &bGNet,
                       const NPNTransformation &t) {
  BoundGNet result = bGNet.clone();
  npnTransformInplace(result, t);
  return result;
}

gate::model::SubnetID npnTransform(const gate::model::Subnet &subnet,
                                   const NPNTransformation &t) {
  using Cell = gate::model::Subnet::Cell;
  using Subnet = gate::model::Subnet;
  using SubnetBuilder = gate::model::SubnetBuilder;

  uint16_t negationMask = t.negationMask;
  NPNTransformation::InputPermutation permutation = t.permutation;
  SubnetBuilder builder;

  const auto &entries = subnet.getEntries();

  // Checking inputs
  size_t expectedInputCount = permutation.size();
  assert(entries.size() >= expectedInputCount);

  NPNTransformation::InputPermutation rPermutation(permutation.size());
  for (size_t i = 0; i < permutation.size(); i++) {
    rPermutation[permutation[i]] = i;
  }

  for (size_t i = 0; i < entries.size(); ++i) {
    const Cell &cell = entries[i].cell;
    if (i < expectedInputCount) {
      assert(cell.isIn() &&
             "Subnet inputs count doesn't match permutation size.");
    }

    Subnet::LinkList links(cell.link, cell.link + cell.arity);
    for (auto &link : links) {
      size_t idx = link.idx;
      if (idx < expectedInputCount) {
        link.idx = rPermutation[idx];
        if (((negationMask >> idx) & 1) == 1) {
          link.inv = 1 - link.inv;
        }
      }
      if (cell.isOut() && (((negationMask >> expectedInputCount) & 1) == 1)) {
        link.inv = 1 - link.inv;
      }
    }
    builder.addCell(cell.getTypeID(), links);
  }

  return builder.make();
}

static TT applyGateFunc(const GateSymbol::Value func,
                        const std::vector<TT> &inputList,
                        const size_t numVars) {
  TT result;
  switch (func) {
  case GateSymbol::ZERO:
    result = TT(numVars);
    break;
  case GateSymbol::ONE:
    result = TT(numVars);
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

TT buildTT(const BoundGNet &bGNet) {
  size_t inputCount = bGNet.inputBindings.size();
  bGNet.net->sortTopologically();
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
        assert(ttMap.find(inputId) != ttMap.end());
        TT inputTT = ttMap[inputId];
        inputList.push_back(inputTT);
      }
      curResult = applyGateFunc(gate->func(), inputList, inputCount);
    }
    if (gate->isTarget()) {
      result = curResult;
    }
    ttMap[gateId] = curResult;
  }
  assert(result.num_vars() == inputCount);
  return result;
}

SOP findAnyLevel0Kernel(const SOP &sop) {
  Cube lit = findAnyRepeatLiteral(sop);
  if (lit._mask == 0u) {
    return sop;
  }
  SOP quotient = findDivideByLiteralQuotient(sop, lit);
  makeCubeFree(quotient);
  return findAnyLevel0Kernel(quotient);
}

Cube findAnyRepeatLiteral(const SOP &sop) {
  uint32_t onesBuf{0};
  uint32_t zerosBuf{0};
  uint32_t ones{0};
  uint32_t zeros{0};
  uint32_t repeatOnes{0};
  uint32_t repeatZeros{0};
  for (const Cube &cube : sop) {
    ones = cube._bits & cube._mask;
    zeros = ~cube._bits & cube._mask;
    repeatOnes = onesBuf & ones;
    repeatZeros = zerosBuf & zeros;
    if (repeatOnes) {
      uint32_t bit = FIRST_ONE_BIT(repeatOnes);
      return Cube{bit, bit};
    }
    if (repeatZeros) {
      uint32_t bit = FIRST_ONE_BIT(repeatZeros);
      return Cube{0u, bit};
    }
    onesBuf |= ones;
    zerosBuf |= zeros;
  }
  return Cube{};
}

SOP findDivideByLiteralQuotient(const SOP &sop, const Cube lit) {
  SOP quotient;
  for (const Cube &cube : sop) {
    if (cubeHasLiteral(cube, lit)) {
      Cube newCube = cube;
      newCube._bits &= ~lit._mask;
      newCube._mask &= ~lit._mask;
      quotient.push_back(newCube);
    }
  }
  return quotient;
}

Cube findCommonCube(const SOP &sop) {
  uint32_t ones{1u};
  uint32_t zeros{1u};
  for (const Cube &cube : sop) {
    ones &= (cube._bits & cube._mask);
    zeros &= (~cube._bits & cube._mask);
  }
  return Cube{ones, ones | zeros};
}

bool cubeFree(const SOP &sop) {
  return (findCommonCube(sop)._mask == 0u);
}

void makeCubeFree(SOP &sop) {
  Cube common = findCommonCube(sop);
  if (common._mask) {
    for (Cube &cube : sop) {
      cube._mask &= ~common._mask;
      cube._bits &= ~common._mask;
    }
  }
}

Cube findBestLiteral(const SOP &sop, Cube lits) {
  size_t max{0u};
  Cube res;
  for (; lits._mask; lits._mask &= (lits._mask - 1)) {
    uint32_t bit = FIRST_ONE_BIT(lits._mask);
    Cube lit(lits._bits & bit, bit);
    size_t count{0};
    for (Cube cube : sop) {
      if (cubeHasLiteral(cube, lit)) {
        ++count;
      }
    }
    if (count > max) {
      max = count;
      res = lit;
    }
  }
  assert(res._mask);
  return res;
}

bool cubeHasLiteral(const Cube cube, const Cube lit) {
  return (cube._mask & lit._mask) && ((cube._bits & lit._mask) == lit._bits);
}

bool cubeContain(Cube large, Cube small) {
  return ((large._mask & small._mask) == small._mask) &&
      ((large._bits & small._mask) == small._bits);
}

Cube cutCube(Cube large, Cube small) {
  return Cube{large._bits & ~small._mask, large._mask & ~small._mask};
}

Link synthFromSOP(const SOP &sop, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, uint16_t maxArity) {
  if (sop.size() == 1) {
    return synthFromCube(sop[0], inputs, subnetBuilder, maxArity);
  }

  LinkList links;
  for (auto it = sop.begin(); it < sop.end(); ++it) {
    Link link = synthFromCube(*it, inputs, subnetBuilder, maxArity);
    links.push_back(~link);
  }

  return ~subnetBuilder.addCellTree(CellSymbol::AND, links, maxArity);
}

Link synthFromCube(Cube cube, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, int16_t maxArity) {
  uint32_t mask {cube._mask};
  LinkList links;
  for (; mask; mask &= (mask - 1)) {
    size_t idx = std::log2(mask - (mask & (mask - 1)));
    bool inv = !(cube.get_bit(idx));
    links.push_back(Link(inputs[idx].idx, inv));
  }
  if (links.size() == 1) {
    return links[0];
  }
  return subnetBuilder.addCellTree(CellSymbol::AND, links, maxArity);
}

} // namespace eda::utils
