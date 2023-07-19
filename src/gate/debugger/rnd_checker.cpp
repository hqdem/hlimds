//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/rnd_checker.h"

using GNet = eda::gate::model::GNet;

namespace eda::gate::debugger {

CheckerResult rndChecker(GNet &miter,
                         const unsigned int tries,
                         const bool exhaustive) {

  if (!(miter.isComb())) {
    LOG_ERROR << "Checker works with combinational circuits only!" << std::endl;
    return CheckerResult::ERROR;
  }

  std::uint64_t outNum = miter.nTargetLinks();

  if (outNum != 1) {
    LOG_ERROR << "Unsupported number of OUT gates: " << outNum << std::endl;
    return CheckerResult::ERROR;
  }

  std::uint64_t inputNum = miter.nSourceLinks();

  if (inputNum < 2 || inputNum > 64) {
    LOG_ERROR << "Unsupported number of inputs: " << inputNum << std::endl;
    return CheckerResult::ERROR;
  }

  auto compiled = makeCompiled(miter);
  std::uint64_t output;
  std::uint64_t inputPower = 1ULL << inputNum;

  if (!exhaustive) {
    for (std::uint64_t t = 0; t < tries; t++) {
      for (std::uint64_t i = 1; i < inputNum; i++) {
        std::uint64_t temp = rand() % inputPower;
        compiled.simulate(output, temp);
        if (output == 1) {
          return CheckerResult::NOTEQUAL;
        }
      }
    }
    return CheckerResult::UNKNOWN;
  }

  if (exhaustive) {
    for (std::uint64_t t = 0; t < inputPower; t++) {
      compiled.simulate(output, t);
      if (output == 1) {
        return CheckerResult::NOTEQUAL;
      }
    }
    return CheckerResult::EQUAL;
  }

  return CheckerResult::ERROR;
}

void RndChecker::setTries(int tries) {
  this->tries = tries;
}

void RndChecker::setExhaustive(bool exhaustive) {
  this->exhaustive = exhaustive;
}

CheckerResult RndChecker::equivalent(GNet &lhs,
                                     GNet &rhs,
                                     Checker::GateIdMap &gmap) {

    GateBinding ibind, obind, tbind;

    // Input-to-input correspondence.
    for (auto oldSourceLink : lhs.sourceLinks()) {
      auto newSourceId = gmap[oldSourceLink.target];
      ibind.insert({oldSourceLink, Gate::Link(newSourceId)});
    }

    // Output-to-output correspondence.
    for (auto oldTargetLink : lhs.targetLinks()) {
      auto newTargetId = gmap[oldTargetLink.source];
      obind.insert({oldTargetLink, Gate::Link(newTargetId)});
    }

    // Trigger-to-trigger correspondence.
    for (auto oldTriggerId : lhs.triggers()) {
      auto newTriggerId = gmap[oldTriggerId];
      tbind.insert({Gate::Link(oldTriggerId), Gate::Link(newTriggerId)});
    }

    Checker::Hints hints;
    hints.sourceBinding  = std::make_shared<GateBinding>(std::move(ibind));
    hints.targetBinding  = std::make_shared<GateBinding>(std::move(obind));
    hints.triggerBinding = std::make_shared<GateBinding>(std::move(tbind));

    GNet *net = miter(lhs, rhs, hints);
    return rndChecker(*net, tries, exhaustive);
  }

} // namespace eda::gate::debugger
