//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/rnd_checker.h"
#include "util/logging.h"

namespace eda::gate::debugger {

void RndChecker::setTries(int tries) {
  this->tries = tries;
}

void RndChecker::setExhaustive(bool exhaustive) {
  this->exhaustive = exhaustive;
}

CheckerResult RndChecker::equivalent(const GNet &lhs,
                                     const GNet &rhs,
                                     const GateIdMap &gmap) const {
  GNet *net = miter(lhs, rhs, gmap);

  if (!(net->isComb())) {
    LOG_DEBUG(LOG_ERROR << "Checker works with combinational circuits only!");
    return CheckerResult::ERROR;
  }

  std::uint64_t outNum = net->nTargetLinks();

  if (outNum != 1) {
    LOG_DEBUG(LOG_ERROR << "Unsupported number of OUT gates: " << outNum);
    return CheckerResult::ERROR;
  }

  std::uint64_t inputNum = net->nSourceLinks();

  if (inputNum < 2 || inputNum >= 64) {
    LOG_DEBUG(LOG_ERROR << "Unsupported number of inputs: " << inputNum);
    return CheckerResult::ERROR;
  }

  auto compiled = makeCompiled(*net);
  std::uint64_t output;
  std::uint64_t inputPower = 1ULL << inputNum;

  if (!exhaustive) {
    for (int t = 0; t < tries; t++) {
      for (std::uint64_t i = 1; i < inputNum; i++) {
        std::uint64_t temp = rand() % inputPower;
        compiled.simulate(output, temp);
        if (output == 1) {
          return CheckerResult(CheckerResult::NOTEQUAL, temp, inputNum);
        }
      }
    }
    return CheckerResult::UNKNOWN;
  }

  if (exhaustive) {
    for (std::uint64_t t = 0; t < inputPower; t++) {
      compiled.simulate(output, t);
      if (output == 1) {
        return CheckerResult(CheckerResult::NOTEQUAL, t, inputNum);
      }
    }
    return CheckerResult::EQUAL;
  }

  return CheckerResult::ERROR;
}

} // namespace eda::gate::debugger
