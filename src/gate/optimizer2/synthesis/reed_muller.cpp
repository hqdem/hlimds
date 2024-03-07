//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "reed_muller.h"

namespace eda::gate::optimizer2::synthesis {

  using Subnet = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID = model::SubnetID;

  SubnetID ReedMuller::synthesize(const DinTruthTable &func,
                                  uint16_t maxArity) {

    Polynomial resultFunction = getTT(func);
    const size_t maxSize = (maxArity < Subnet::Cell::InPlaceLinks) ? 
                 maxArity : Subnet::Cell::InPlaceLinks;
                 
    SubnetBuilder subnetBuilder;
    uint64_t argNum  = resultFunction[resultFunction.size() - 1];
    std::vector<size_t> idx (argNum);
    LinkList resultOutput;

    for (size_t i = 0; i < argNum; ++i) {
      idx[i] = subnetBuilder.addInput().idx;
    }

    if (resultFunction[0]) {
      resultOutput.push_back(subnetBuilder.addCell(model::ONE));
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

      resultOutput.push_back(subnetBuilder.addCellTree(model::AND, currentNode, maxSize));
    }

    LinkList outputNodes;
    Link out;

    if (resultOutput.size() >= 2) {
      out = subnetBuilder.addCellTree(model::XOR, resultOutput, maxSize);
    }

    else if (resultOutput.size() == 1) {
      out = resultOutput[0];
    }

    subnetBuilder.addOutput(out);
    return subnetBuilder.make();
  }

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
} // namespace eda::gate::optimizer2::synthesis
