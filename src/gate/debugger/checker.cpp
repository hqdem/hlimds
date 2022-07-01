//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/checker.h"
#include "gate/debugger/encoder.h"

#include <cassert>

namespace eda::gate::debugger {

bool Checker::areEqual(const GNet &lhs,
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
  return false;
}

bool Checker::areEqualHier(const GNet &lhs,
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

    if (!areEqual(*lhsSubnet, *rhsSubnet, hintsSubnets)) {
      return false;
    }
  }

  return true;
}

bool Checker::areEqualComb(const GNet &lhs,
                           const GNet &rhs,
                           const GateBinding &ibind,
                           const GateBinding &obind) const {
  return areEqualComb({ &lhs, &rhs }, nullptr, ibind, obind);
}

bool Checker::areEqualSeq(const GNet &lhs,
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

    assert(lhsTrigger->kind()  == rhsTrigger->kind());
    assert(lhsTrigger->arity() == rhsTrigger->arity());

    imap.insert({Gate::Link(lhsTrigger->id()), Gate::Link(rhsTrigger->id())});

    for (std::size_t i = 0; i < lhsTrigger->arity(); i++) {
      const Signal lhsInput = lhsTrigger->input(i);
      const Signal rhsInput = rhsTrigger->input(i);

      omap.insert({Gate::Link(lhsInput), Gate::Link(rhsInput)});
    }
  }

  return areEqualComb(lhs, rhs, imap, omap);
}

bool Checker::areEqualSeq(const GNet &lhs,
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
    connectTo.insert({encInLink.source, lhsTrigger->input(0).gateId()});
  }

  // Connect the LHS-trigger outputs to the decoder outputs.
  for (const auto &[lhsTriLink, decOutLink] : lhsTriDecOut) {
    connectTo.insert({lhsTriLink.source, decOutLink.source});
  }

  // Append the encoder outputs and the RHS-trigger inputs to the outputs.
  for (const auto &[rhsTriLink, encOutLink] : rhsTriEncOut) {
    const auto *rhsTrigger = Gate::get(rhsTriLink.source);
    omap.insert({encOutLink, Gate::Link(rhsTrigger->input(0).gateId())});
  }

  // Append the decoder inputs and the RHS-trigger outputs to to the inputs.
  for (const auto &[rhsTriLink, decInLink] : rhsTriDecIn) {
    imap.insert({decInLink, rhsTriLink});
  }

  return areEqualComb({&lhs, &rhs, &enc, &dec}, &connectTo, imap, omap);
}

bool Checker::areEqualComb(const std::vector<const GNet*> &nets,
                           const GateConnect *connectTo,
                           const GateBinding &ibind,
                           const GateBinding &obind) const {
  Encoder encoder;
  encoder.setConnectTo(connectTo);

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

  if (!verdict) {
    error(encoder.context(), ibind, obind);
  }

  return verdict;
}

void Checker::error(Context &context,
                    const GateBinding &ibind,
                    const GateBinding &obind) const {
  bool comma;
  context.dump("miter.cnf");

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

} // namespace eda::gate::debugger
