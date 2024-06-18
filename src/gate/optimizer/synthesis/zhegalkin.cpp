//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "zhegalkin.h"

namespace eda::gate::optimizer::synthesis {

using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

SubnetID createScheme(Polynomial &resultFunction, 
    Polarization &polarization, uint64_t maxArity, uint64_t argNum) {

  SubnetBuilder subnetBuilder;
  Link one;

  if (argNum == 2) {
    bool emptyScheme = true;
    for (size_t i = 0; i < (1ull << argNum); ++i) {
      if (resultFunction[i]) {
        emptyScheme = false;
        break;
      }
    }

    if (emptyScheme) {
      subnetBuilder.addInputs(2);
      subnetBuilder.addOutput(subnetBuilder.addCell(model::ZERO));
      return subnetBuilder.make();
    }
  }

  const size_t maxSize = (maxArity < Subnet::Cell::InPlaceLinks) ? 
      maxArity : Subnet::Cell::InPlaceLinks;

  std::vector<size_t> idx (argNum);
  LinkList resultOutput;

  for (size_t i = 0; i < argNum; ++i) {
    idx[i] = subnetBuilder.addInput().idx;
  }

  for (size_t i = 0; i < argNum; ++i) {
    if (polarization[i]) {
      idx[i] = subnetBuilder.addCell(model::BUF, ~Link(idx[i])).idx;
    }
  }

  if (resultFunction[0]) {
    one = subnetBuilder.addCell(model::ONE);
    resultOutput.push_back(one);
  }

  for (int i = 1; i < (1 << argNum); ++i) {
    if (!resultFunction[i]) {
      continue;
    }

    std::vector<int> temporaryInputNodes = eda::utils::popcnt(i);
    LinkList currentNode;

    for (auto n : temporaryInputNodes) {
      currentNode.push_back(Link(idx[n]));
    }
    resultOutput.push_back(
        subnetBuilder.addCellTree(model::AND, currentNode, maxSize));
  }

  LinkList outputNodes;
  Link out;

  if (resultOutput.size() >= 2) {
    out = subnetBuilder.addCellTree(model::XOR, resultOutput, maxSize);
  } else if (resultOutput.size() == 1) {
    out = resultOutput[0];
  }

  subnetBuilder.addOutput(out);
  return subnetBuilder.make();
}

SubnetID ZhegalkinSynthesizer::synthesize(const TruthTable &func,
                                          const TruthTable &,
                                          uint16_t maxArity) const {

  Polynomial resultFunction = getTT(func);
  Polarization polarization = Polarization(resultFunction.size(), false);
  return createScheme(resultFunction, polarization, maxArity, func.num_vars());
}

Polynomial ZhegalkinSynthesizer::getTT(const TruthTable &t) const {
  Polynomial charFunction = charFromTruthTable(t);
  Polynomial resultFunction = charFromFunction(charFunction);
  return resultFunction;
}

uint64_t ZhegalkinSynthesizer::apply(const Polynomial &func,
    const std::string &s) const {

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

Polynomial ZhegalkinSynthesizer::charFromTruthTable(const TruthTable &t) const {
    
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

Polynomial ZhegalkinSynthesizer::charFromFunction(
    Polynomial &func) const {

  uint64_t numVar = func[func.size() - 1];
  uint64_t numBits = 1 << numVar;
  Polynomial resultFunction(numBits + 1);

  for (uint64_t i = 0; i < numBits; ++i) {
    resultFunction[i] = apply(func, eda::utils::toBinString(i, numVar));
  }

  resultFunction[resultFunction.size() - 1] = numVar;
  return resultFunction;
}

} // namespace eda::gate::optimizer::synthesis
