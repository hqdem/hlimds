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

CheckerResult RndChecker2::equivalent(const Subnet &lhs,
                                      const Subnet &rhs,
                                      const CellToCell &gmap) const {
  if (lhs.getInNum() != rhs.getInNum()) {
    LOG_ERROR << "Nets have different number of inputs." << std::endl;
    return CheckerResult::ERROR;
  }
  if (lhs.getOutNum() != rhs.getOutNum()) {
    LOG_ERROR << "Nets have different number of inputs." << std::endl;
    return CheckerResult::ERROR;
  }

  const Subnet &miter = miter2(lhs, rhs, gmap);
  assert(miter.getOutNum() == 1);

  const auto inputNum = miter.getInNum();

  Simulator simulator(miter);
  Simulator::DataVector values(inputNum);

  if (!exhaustive) {
    for (uint64_t t = 0; t < tries; t++) {
      for (uint64_t i = 0; i < inputNum; i++) {
        values[i] = std::rand();
      }
      simulator.simulate(values);
      const auto output = simulator.getValue(miter.getOut(0));
      if (output) {
        return CheckerResult(CheckerResult::NOTEQUAL, values);
      }
    }
    return CheckerResult::UNKNOWN;
  }

  if (exhaustive) {
    if (inputNum > 32) {
      LOG_ERROR << "Unsupported number of inputs: " << inputNum << std::endl;
      return CheckerResult::ERROR;
    }

    const auto inputPower = (1ull << inputNum);

    size_t iterations = (inputPower <= 64) ? 1ull : (inputPower >> 6);
    for (size_t i = 0; i < iterations; i++) {
      simulator.simulate(getAllValues(inputNum, i));
      const auto output = simulator.getValue(miter.getOut(0));
      if (output) {
        return CheckerResult(CheckerResult::NOTEQUAL, values);
      }
    }
    return CheckerResult::EQUAL;
  }

  return CheckerResult::ERROR;
}

} // namespace eda::gate::debugger2
