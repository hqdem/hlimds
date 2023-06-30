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

static simulator::Simulator simulator;

CheckerResult rndChecker(GNet &miter,
                         const unsigned int tries,
                         const bool exhaustive) {

  if (!(miter.isComb())) {
    return CheckerResult::ERROR;
  }

  // check the number of outputs
  assert(miter.nTargetLinks() == 1);

  std::uint64_t inputNum = miter.nSourceLinks();
  assert(inputNum >= 2 && inputNum <= 64);

  GNet::In gnetInput(1);
  auto &input = gnetInput[0];

  for (auto srcLink : miter.sourceLinks()) {
    input.push_back(srcLink.target);
  }

  Gate::SignalList inputs;
  Gate::Id outputId = model::Gate::INVALID;
  GNet::LinkList in;
  for (auto *gate : miter.gates()) {
    if (gate->isTarget()) {
      outputId = gate->id();
      break;
    }
  }

  if (outputId == model::Gate::INVALID) {
    LOG_ERROR << "Can't find OUT gate at miter!";
    return CheckerResult::ERROR;
  }

  for (size_t n = 0; n < inputNum; n++) {
    in.push_back(GNet::Link(input[n]));
  }

  GNet::LinkList out{Gate::Link(outputId)};

  for (auto input : inputs) {
    in.push_back(GNet::Link(input.node()));
  }

  miter.sortTopologically();
  auto compiled = simulator.compile(miter, in, out);
  std::uint64_t output;
  std::uint64_t inputPower = static_cast<std::uint64_t>(1ULL << inputNum);
  
  if (!exhaustive) {
    for (std::uint64_t t = 0; t < tries; t++) {
      for (std::uint64_t i = 0; i < (inputNum - 1); i++) {
        std::uint64_t temp = rand() % inputPower;
        compiled.simulate(output, temp);
        if (output == 1) {
          return  CheckerResult::NOTEQUAL;
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
