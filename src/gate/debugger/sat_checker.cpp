//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/debugger/encoder.h"
#include "gate/debugger/miter.h"
#include "gate/simulator/simulator.h"

#include <cassert>

namespace eda::gate::debugger {

CheckerResult SatChecker::equivalent(const GNet &lhs,
                                     const GNet &rhs,
                                     const GateIdMap &gmap) const {
  SatChecker::Hints hints = makeHints(lhs, rhs, gmap);
  return equivalent(lhs, rhs, hints);
}

CheckerResult SatChecker::equivalent(const GNet &lhs,
                                     const GNet &rhs,
                                     const Hints &hints) const {
  const unsigned flatCheckBound = 64 * 1024;

  assert(hints.isKnownIoPortBinding());
  assert(lhs.nSourceLinks() == rhs.nSourceLinks());
  assert(lhs.nSourceLinks() <= hints.sourceBinding->size());
  assert(rhs.nTargetLinks() <= hints.targetBinding->size());

  if (hints.isKnownSubnetBinding() &&
      lhs.nGates() + rhs.nGates() > 2 * flatCheckBound) {
    return areEqualHier(lhs, rhs, hints);
  }

  assert(lhs.isComb() == rhs.isComb());

  if (lhs.isComb() && rhs.isComb()) {
    return areEqualComb(lhs, rhs,
                       *hints.sourceBinding,
                       *hints.targetBinding);
  }

  if (hints.isKnownTriggerBinding()) {
    return areEqualSeq(lhs, rhs,
                      *hints.sourceBinding,
                      *hints.targetBinding,
                      *hints.triggerBinding);
  }

  if (hints.isKnownStateEncoding()) {
    return areEqualSeq(lhs, rhs,
                      *hints.encoder,
                      *hints.decoder,
                      *hints.sourceBinding,
                      *hints.targetBinding,
                      *hints.lhsTriEncIn,
                      *hints.lhsTriDecOut,
                      *hints.rhsTriEncOut,
                      *hints.rhsTriDecIn);
  }

  assert(false && "Unimplemented LEC");
  return CheckerResult::ERROR;
}

CheckerResult SatChecker::areEqualHier(const GNet &lhs,
                                       const GNet &rhs,
                                       const Hints &hints) const {
  assert(!lhs.isFlat() && !rhs.isFlat());
  assert(lhs.nSubnets() == rhs.nSubnets());
  assert(lhs.nSubnets() == hints.subnetBinding->size());
  assert(hints.isKnownInnerBinding());

  for (const auto &[lhsSubnetId, rhsSubnetId] : *hints.subnetBinding) {
    const auto *lhsSubnet = lhs.subnet(lhsSubnetId);
    const auto *rhsSubnet = rhs.subnet(rhsSubnetId);


    GateBinding imap;
    for (auto lhsLink : lhsSubnet->sourceLinks()) {
      const auto &binding = lhs.hasSourceLink(lhsLink) ? *hints.sourceBinding
                                                       : *hints.innerBinding;
      auto i = binding.find(lhsLink);
      assert(i != binding.end());
      imap.insert({lhsLink, i->second});
    }

    GateBinding omap;
    for (auto lhsLink : lhsSubnet->targetLinks()) {
      const auto &binding = lhs.hasTargetLink(lhsLink) ? *hints.targetBinding
                                                       : *hints.innerBinding;
      auto i = binding.find(lhsLink);
      assert(i != binding.end());
      imap.insert({lhsLink, i->second});
    }

    Hints hintsSubnets;
    hintsSubnets.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
    hintsSubnets.targetBinding = std::make_shared<GateBinding>(std::move(omap));
    hintsSubnets.innerBinding  = hints.innerBinding;

    CheckerResult result = equivalent(*lhsSubnet, *rhsSubnet, hintsSubnets);
    if (result.notEqual()) {
      return CheckerResult::NOTEQUAL;
    }
  }

  return CheckerResult::EQUAL;
}

CheckerResult SatChecker::areEqualComb(const GNet &lhs,
                                       const GNet &rhs,
                                       const GateBinding &ibind,
                                       const GateBinding &obind) const {
  if (lhs.nSourceLinks() <= simCheckBound) {
    return areEqualCombSim(lhs, rhs, ibind, obind);
  }

  return areEqualCombSat({ &lhs, &rhs }, nullptr, ibind, obind);
}

CheckerResult SatChecker::isEqualCombMiter(const GNet &miter) const {
  assert(miter.isComb());
  assert(miter.nOuts() == 1);

  if (miter.nSourceLinks() <= simCheckBound) {
    return isEqualCombSimMiter(miter);
  }
  return isEqualCombSatMiter(miter);
}

CheckerResult SatChecker::areEqualSeq(const GNet &lhs,
                                      const GNet &rhs,
                                      const GateBinding &ibind,
                                      const GateBinding &obind,
                                      const GateBinding &tbind) const {
  GateBinding imap(ibind);
  GateBinding omap(obind);

  // Cut triggers.
  for (const auto &[lhsLink, rhsLink] : tbind) {
    const Gate *lhsTrigger = Gate::get(lhsLink.source);
    const Gate *rhsTrigger = Gate::get(rhsLink.source);

    assert(lhsTrigger->func()  == rhsTrigger->func());
    assert(lhsTrigger->arity() == rhsTrigger->arity());

    imap.insert({Gate::Link(lhsTrigger->id()), Gate::Link(rhsTrigger->id())});

    for (std::size_t i = 0; i < lhsTrigger->arity(); i++) {
      const Gate::Signal lhsInput = lhsTrigger->input(i);
      const Gate::Signal rhsInput = rhsTrigger->input(i);

      omap.insert({Gate::Link(lhsInput.node()), Gate::Link(rhsInput.node())});
    }
  }

  return areEqualComb(lhs, rhs, imap, omap);
}

CheckerResult SatChecker::areEqualSeq(const GNet &lhs,
                                      const GNet &rhs,
                                      const GNet &enc,
                                      const GNet &dec,
                                      const GateBinding &ibind,
                                      const GateBinding &obind,
                                      const GateBinding &lhsTriEncIn,
                                      const GateBinding &lhsTriDecOut,
                                      const GateBinding &rhsTriEncOut,
                                      const GateBinding &rhsTriDecIn) const {

  //=========================================//
  //                                         //
  //   inputs---------inputs                 //
  //    LHS'           RHS'                  //
  //     |              |                    //
  //   encode           |                    //
  //     |--------------|---------- outputs' //
  // (triggers)     (triggers)               //
  //     |--------------|---------- inputs'  //
  //   decode           |                    //
  //     |              |                    //
  //    LHS''          RHS''                 //
  //  outputs--------outputs                 //
  //                                         //
  //=========================================//

  GateConnect connectTo;
  GateBinding imap(ibind);
  GateBinding omap(obind);

  // Connect the encoder inputs to the LHS-trigger D inputs' drivers.
  for (const auto &[lhsTriLink, encInLink] : lhsTriEncIn) {
    const auto *lhsTrigger = Gate::get(lhsTriLink.source);
    connectTo.insert({encInLink.source, lhsTrigger->input(0).node()});
  }

  // Connect the LHS-trigger outputs to the decoder outputs.
  for (const auto &[lhsTriLink, decOutLink] : lhsTriDecOut) {
    connectTo.insert({lhsTriLink.source, decOutLink.source});
  }

  // Append the encoder outputs and the RHS-trigger inputs to the outputs.
  for (const auto &[rhsTriLink, encOutLink] : rhsTriEncOut) {
    const auto *rhsTrigger = Gate::get(rhsTriLink.source);
    omap.insert({encOutLink, Gate::Link(rhsTrigger->input(0).node())});
  }

  // Append the decoder inputs and the RHS-trigger outputs to to the inputs.
  for (const auto &[rhsTriLink, decInLink] : rhsTriDecIn) {
    imap.insert({decInLink, rhsTriLink});
  }

  return areEqualCombSat({&lhs, &rhs, &enc, &dec}, &connectTo, imap, omap);
}

CheckerResult SatChecker::areEqualCombSim(const GNet &lhs,
                                          const GNet &rhs,
                                          const GateBinding &ibind,
                                          const GateBinding &obind) const {
  assert(lhs.nSourceLinks() == rhs.nSourceLinks());
  assert(lhs.nSourceLinks() <= simCheckBound);

  GNet::LinkList lhsInputs;
  GNet::LinkList rhsInputs;

  lhsInputs.reserve(ibind.size());
  rhsInputs.reserve(ibind.size());

  for (const auto &[lhsLink, rhsLink] : ibind) {
    lhsInputs.push_back(lhsLink);
    rhsInputs.push_back(rhsLink);
  }

  GNet::LinkList lhsOutputs;
  GNet::LinkList rhsOutputs;

  lhsOutputs.reserve(obind.size());
  rhsOutputs.reserve(obind.size());

  for (const auto &[lhsLink, rhsLink] : obind) {
    lhsOutputs.push_back(lhsLink);
    rhsOutputs.push_back(rhsLink);
  }

  eda::gate::simulator::Simulator simulator;

  auto lhsCompiled = simulator.compile(lhs, lhsInputs, lhsOutputs);
  auto rhsCompiled = simulator.compile(rhs, rhsInputs, rhsOutputs);

  for (uint64_t in = 0; in < (1ull << lhs.nSourceLinks()); in++) {
    uint64_t lhsOut, rhsOut;

    lhsCompiled.simulate(lhsOut, in);
    rhsCompiled.simulate(rhsOut, in);

    if (lhsOut != rhsOut) {
      return CheckerResult(CheckerResult::NOTEQUAL, in, lhs.nSourceLinks());
    }
  }

  return CheckerResult::EQUAL;
}

CheckerResult SatChecker::isEqualCombSimMiter(const GNet &miter) const {
  std::uint64_t inputNum = miter.nSourceLinks();
  assert(inputNum <= simCheckBound);

  auto compiled = makeCompiled(miter);
  std::uint64_t output;
  std::uint64_t inputPower = 1ULL << inputNum;

  for (std::uint64_t t = 0; t < inputPower; t++) {
    compiled.simulate(output, t);
    if (output == 1) {
      return CheckerResult(CheckerResult::NOTEQUAL, t, inputNum);
    }
  }
    return CheckerResult::EQUAL;
}

CheckerResult SatChecker::areEqualCombSat(const std::vector<const GNet*> &nets,
                                          const GateConnect *connectTo,
                                          const GateBinding &ibind,
                                          const GateBinding &obind) const {
  Encoder encoder;

  // Equate the inputs.
  for (const auto &[lhsGateLink, rhsGateLink] : ibind) {
    const auto x = encoder.var(lhsGateLink.source, 0);
    const auto y = encoder.var(rhsGateLink.source, 0);

    encoder.encodeBuf(y, x, true);
  }

  // Encode the nets.
  for (const auto *net : nets) {
    encoder.encode(*net, 0);
  }

  // Compare the outputs.
  Context::Clause existsDiff;
  for (const auto &[lhsGateLink, rhsGateLink] : obind) {
    const auto y  = encoder.newVar();
    const auto x1 = encoder.var(lhsGateLink.source, 0);
    const auto x2 = encoder.var(rhsGateLink.source, 0);

    encoder.encodeXor(y, x1, x2, true, true, true);
    existsDiff.push(Context::lit(y, true));
  }

  // (lOut[1] != rOut[1]) || ... || (lOut[m] != rOut[m]).
  encoder.encode(existsDiff);

  const auto verdict = !encoder.solve();

  if (verdict) {
    return CheckerResult::EQUAL;
  }

 #ifdef UTOPIA_DEBUG
  error(encoder.context(), ibind, obind);
 #endif

  auto &cont = encoder.context();
  std::vector<bool> counterEx;
  for (const auto &[lhsLink, rhsLink] : ibind) {
    counterEx.push_back(cont.value(cont.var(lhsLink.source, 0)));
  }

  return CheckerResult(CheckerResult::NOTEQUAL, counterEx);
}

CheckerResult SatChecker::isEqualCombSatMiter(const GNet &miter) const {
  Encoder encoder;
  encoder.setConnectTo(nullptr);

  if (miter.nOuts() != 1) {
    LOG_ERROR << "Incorrect number of OUT gates at miter!" << std::endl;
    return CheckerResult::ERROR;
  }

  // Encode the miter.
  encoder.encode(miter, 0);

  GateId outputId = (*miter.targetLinks().begin()).source;

  // Compare the outputs.
  const auto y = encoder.var(outputId, 0);
  encoder.encodeFix(y, 1);

  const auto verdict = encoder.solveLimited();

  std::vector<bool> counterEx;
  if (verdict) {
    auto &cont = encoder.context();
    for (const auto link : miter.sourceLinks()) {
      counterEx.push_back(cont.value(cont.var(link.source, 0)));
    }
  }

  return verdict ? CheckerResult(CheckerResult::NOTEQUAL, counterEx) :
                   CheckerResult::EQUAL;
}

void SatChecker::error(Context &context,
                       const GateBinding &ibind,
                       const GateBinding &obind) const {
  bool comma;
#ifdef UTOPIA_DEBUG
  context.dump("miter.cnf");
#endif

  comma = false;
  std::cout << "Inputs: ";
  for (const auto &[lhsGateLink, rhsGateLink] : ibind) {
    if (comma) std::cout << ", ";
    comma = true;

    std::cout << context.value(context.var(lhsGateLink.source, 0)) << "|";
    std::cout << context.value(context.var(rhsGateLink.source, 0));
  }
  std::cout << std::endl;

  comma = false;
  std::cout << "Outputs: ";
  for (const auto &[lhsGateLink, rhsGateLink] : obind) {
    if (comma) std::cout << ", ";
    comma = true;

    std::cout << context.value(context.var(lhsGateLink.source, 0)) << "|";
    std::cout << context.value(context.var(rhsGateLink.source, 0));
  }
  std::cout << std::endl;
}

SatChecker::Hints makeHints(const GNet &lhs,
                            const GNet &rhs,
                            const GateIdMap &gmap) {
  SatChecker::GateBinding ibind, obind, tbind;

  // Input-to-input correspondence.
  for (auto oldSourceLink : lhs.sourceLinks()) {
    auto newSourceId = gmap.at(oldSourceLink.target);
    ibind.insert({oldSourceLink, Gate::Link(newSourceId)});
  }

  // Output-to-output correspondence.
  for (auto oldTargetLink : lhs.targetLinks()) {
    GateId source = oldTargetLink.source;
    if (gmap.count(source)) {
      auto newTargetId = gmap.at(source);
      obind.insert({oldTargetLink, Gate::Link(newTargetId)});
    }
  }

  // Trigger-to-trigger correspondence.
  for (auto oldTriggerId : lhs.triggers()) {
    auto newTriggerId = gmap.at(oldTriggerId);
    tbind.insert({Gate::Link(oldTriggerId), Gate::Link(newTriggerId)});
  }

  SatChecker::Hints hints;
  hints.sourceBinding = std::make_shared<SatChecker::GateBinding>(
                        std::move(ibind));
  hints.targetBinding = std::make_shared<SatChecker::GateBinding>(
                        std::move(obind));
  hints.triggerBinding = std::make_shared<SatChecker::GateBinding>(
                        std::move(tbind));

  return hints;
}

} // namespace eda::gate::debugger
