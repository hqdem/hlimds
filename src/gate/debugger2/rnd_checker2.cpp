//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/rnd_checker2.h"

#include <bitset>

namespace eda::gate::debugger2 {

Simulator::DataVector getAllValues(size_t nIn, size_t count) {
  size_t startValue = count * 64;
  std::vector<std::bitset<64>> vec(nIn);
  for (size_t i = startValue; i < startValue + 64; i++) {
    std::bitset<64> current(i);
    for (size_t j = 0; j < vec.size(); j++) {
      if (current.test(j)) {
        vec[j].set(i - startValue);
      }
    }
  }
  Simulator::DataVector res;
  for (size_t k = 0; k < vec.size(); k++) {
    res.push_back(vec[k].to_ullong());
  }
  return res;
}

void RndChecker2::setTries(int tries) {
  this->tries = tries;
}

void RndChecker2::setExhaustive(bool exhaustive) {
  this->exhaustive = exhaustive;
}

CheckerResult RndChecker2::equivalent(Subnet &lhs,
                                      Subnet &rhs,
                                      CellToCell &gmap) {
  if (lhs.getInNum() != rhs.getInNum()) {
    LOG_ERROR << "Nets have different number of inputs." << std::endl;
    return CheckerResult::ERROR;
  }
  if (lhs.getOutNum() != rhs.getOutNum()) {
    LOG_ERROR << "Nets have different number of inputs." << std::endl;
    return CheckerResult::ERROR;
  }

  MiterHints hints = makeHints(lhs, gmap);
  Subnet miter = miter2(lhs, rhs, hints);

  std::uint64_t outNum = miter.getOutNum();

  if (outNum != 1) {
    LOG_ERROR << "Unsupported number of OUT gates: " << outNum << std::endl;
    return CheckerResult::ERROR;
  }
  std::uint64_t inputNum = miter.getInNum();

  if (inputNum < 2 || inputNum > 64) {
    LOG_ERROR << "Unsupported number of inputs: " << inputNum << std::endl;
    return CheckerResult::ERROR;
  }

  std::uint64_t output;
  std::uint64_t inputPower = 1ULL << inputNum;
  Simulator simulator(miter);
  Simulator::DataVector values(inputNum);

  if (!exhaustive) {
    for (std::uint64_t t = 0; t < tries; t++) {
      for (std::uint64_t i = 0; i < inputNum; i++) {
        values[i] = std::rand() % inputPower;
      }
      simulator.simulate(values);
      output = simulator.getValue(miter.getEntries().size() - 1);
      if (output) {
        return CheckerResult(CheckerResult::NOTEQUAL, values);
      }
    }
    return CheckerResult::UNKNOWN;
  }

  if (exhaustive) {
    size_t iterations = (inputPower <= 64) ? 1ULL : (inputPower >> 6);
    for (size_t i = 0; i < iterations; i++) {
      simulator.simulate(getAllValues(inputNum, i));
      output = simulator.getValue(miter.getEntries().size() - 1);
      if (output) {
        return CheckerResult(CheckerResult::NOTEQUAL, values);
      }
    }
    return CheckerResult::EQUAL;
  }

  return CheckerResult::ERROR;
}

} // namespace eda::gate::debugger2
