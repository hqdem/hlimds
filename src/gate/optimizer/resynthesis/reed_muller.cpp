//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "reed_muller.h"
#include "util/arith.h"

#include <algorithm>

namespace eda::gate::optimizer::resynthesis {

ReedMuller::ReedMuller() = default;

ReedMuller::~ReedMuller() = default;

Polynomial ReedMuller::getTT(const DinTruthTable &t) {
  Polynomial charFunction = charFromTruthTable(t);
  Polynomial resultFunction = charFromFunction(charFunction);
  return resultFunction;
}

uint64_t ReedMuller::apply(const Polynomial &func, const std::string &s) {
  std::string sCopy = s;
  if (func[func.size() - 1] < sCopy.size()) {
    assert("Too many arguments for the function");
  }

  while (func[func.size() - 1] > sCopy.size()) {
    sCopy.push_back('0');
  }
  std::reverse(sCopy.begin(), sCopy.end());
  std::vector<int> positionsOfOnes;
  uint64_t res = 0;
  for (size_t i = 0; i < sCopy.size(); ++i) {
    if (sCopy[i] == '1') {
      positionsOfOnes.push_back(i);
    }
  }

  uint64_t size = positionsOfOnes.size();
  uint64_t base = 1 << size;
  for (uint64_t i = 0; i < base; ++i) {
    std::string str = eda::utils::toBinString(i, size);
    int pos = 0;
    for (uint64_t j = 0; j < size; ++j) {
      if (str[j] == '1') {
        pos += 1 << positionsOfOnes[size - 1 - j];
      }
    }
    res ^= func[pos];
  }
  return res;
}

Polynomial ReedMuller::charFromTruthTable(const DinTruthTable &t) {
  Polynomial charFunction;
  uint64_t numBits = t.num_bits();
  uint64_t numVar = t.num_vars();
  charFunction.resize(numBits + 1);

  for (uint64_t i = 0; i < numBits; ++i) {
    charFunction[i] = kitty::get_bit(t, i);
  }

  charFunction[charFunction.size() - 1] = numVar;
  return charFunction;
}

Polynomial ReedMuller::charFromFunction(Polynomial &func) {
  uint64_t numVar = func[func.size() - 1];
  uint64_t numBits = 1 << numVar;
  Polynomial resultFunction(numBits + 1);

  for (uint64_t i = 0; i < numBits; ++i) {
    resultFunction[i] = apply(func, eda::utils::toBinString(i, numVar));
  }

  resultFunction[resultFunction.size() - 1] = numVar;
  return resultFunction;
}

std::shared_ptr<GNet> ReedMuller::getGNet(const DinTruthTable &t) {
  Polynomial func = getTT(t);
  bool flag = func[0] == 1;
  Gate::SignalList inputs;
  Gate::Id outputId;
  uint64_t argNum  = func[func.size() - 1];
  auto net = std::make_shared<GNet>();
  std::vector<Gate::Id> varId(argNum);
  Gate::SignalList output;
  for (size_t i = 0; i < argNum; ++i) {
    varId[i] = net->addIn();
    inputs.push_back(Gate::Signal::always(varId[i]));
  }

  for (size_t i = 1; i < func.size() - 1; ++i) {
    if (!func[i]) {
      continue;
    }
    std::vector<int> variables = popcnt(i);
    Gate::SignalList curVars;
    for (int variable : variables) {
      curVars.push_back(inputs[variable]);
    }
    Gate::Id id;
    if (flag) {
      id = net->addNand(curVars);
      flag = false;
    } else {
      id = net->addAnd(curVars);
    }
    output.emplace_back(Gate::Signal::always(id));
  }

  Gate::Id gateId;
  if (!output.empty()) {
    gateId = net->addXor(output);
  } else if (func[0] == 1) {
    gateId = net->addOne();
  } else {
    gateId = net->addZero();
  }
  outputId = net->addOut(gateId);
  Gate::Signal::always(outputId);
  net->sortTopologically();
  return net;
}

std::vector<int> ReedMuller::popcnt(int a) {
  std::vector<int> res;
  int count = 0;

  while (a) {
    int mask = 0x1;
    mask &= a;
    if (mask == 1) {
      res.push_back(count);
    }
    ++count;
    a = a >> 1;
  }
  return res;
}

} // namespace eda::gate::optimizer::resynthesis
