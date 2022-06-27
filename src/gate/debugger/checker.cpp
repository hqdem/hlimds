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
                       const GateBindList &ibind,
                       const GateBindList &obind,
                       const Hints &hints) const {

  if (lhs.nSources() != rhs.nSources() ||
      lhs.nTargets() != rhs.nTargets()) {
    return false;
  }

  assert(lhs.nSources() == ibind.size());
  assert(rhs.nTargets() == obind.size());

  if (lhs.isComb() && rhs.isComb()) {
    return areEqualComb(lhs, rhs, ibind, obind);
  }

  // TODO:
  return false;
}

bool Checker::areEqualComb(const GNet &lhs,
                           const GNet &rhs,
                           const Checker::GateBindList &ibind,
                           const Checker::GateBindList &obind) const {
  return areEqualComb({ &lhs, &rhs }, nullptr, ibind, obind);
}

bool Checker::areEqualSeq(const GNet &lhs,
                          const GNet &rhs,
                          const GateBindList &ibind,
                          const GateBindList &obind,
                          const GateBindList &tbind) const {
  GateBindList imap(ibind);
  GateBindList omap(obind);

  // Cut triggers.
  for (const auto &[lhsTriggerId, rhsTriggerId] : tbind) {
    const Gate *lhsTrigger = Gate::get(lhsTriggerId);
    const Gate *rhsTrigger = Gate::get(rhsTriggerId);

    if (lhsTrigger->kind() != rhsTrigger->kind()) {
      return false;
    }

    imap.push_back({ lhsTrigger->id(), rhsTrigger->id() });

    assert(lhsTrigger->arity() == rhsTrigger->arity());
    for (std::size_t i = 0; i < lhsTrigger->arity(); i++) {
      const Signal lhsInput = lhsTrigger->input(i);
      const Signal rhsInput = rhsTrigger->input(i);

      omap.push_back({ lhsInput.gateId(), rhsInput.gateId() });
    }
  }

  return areEqualComb(lhs, rhs, imap, omap);
}

bool Checker::areEqualSeq(const GNet &lhs,
                          const GNet &rhs,
                          const GNet &enc,
                          const GNet &dec,
                          const GateBindList &ibind,
                          const GateBindList &obind,
                          const GateBindList &lhsTriEncIn,
                          const GateBindList &lhsTriDecOut,
                          const GateBindList &rhsTriEncOut,
                          const GateBindList &rhsTriDecIn) const {
  
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

  Context::GateIdMap connectTo;

  GateBindList imap(ibind);
  GateBindList omap(obind);

  // Connect the encoder inputs to the LHS-trigger D inputs' drivers.
  for (const auto &[lhsTriId, encInId] : lhsTriEncIn) {
    connectTo.insert({ encInId, Gate::get(lhsTriId)->input(0).gateId() });
  }

  // Connect the LHS-trigger outputs to the decoder outputs.
  for (const auto &[lhsTriId, decOutId] : lhsTriDecOut) {
    connectTo.insert({ lhsTriId, decOutId });
  }

  // Append the encoder outputs and the RHS-trigger inputs to the outputs.
  for (const auto &[rhsTriId, encOutId] : rhsTriEncOut) {
    omap.push_back({ encOutId, Gate::get(rhsTriId)->input(0).gateId() });
  }

  // Append the decoder inputs and the RHS-trigger outputs to to the inputs.
  for (const auto &[rhsTriId, decInId] : rhsTriDecIn) {
    imap.push_back({ decInId, rhsTriId });
  }

  return areEqualComb({ &lhs, &rhs, &enc, &dec }, &connectTo, imap, omap);
}

bool Checker::areEqualComb(const std::vector<const GNet*> &nets,
                           const Checker::GateIdMap *connectTo,
                           const Checker::GateBindList &ibind,
                           const Checker::GateBindList &obind) const {
  Encoder encoder;
  encoder.setConnectTo(connectTo);

  // Equate the inputs.
  for (const auto &[lhsGateId, rhsGateId] : ibind) {
    const auto x = encoder.var(lhsGateId, 0);
    const auto y = encoder.var(rhsGateId, 0);

    encoder.encodeBuf(y, x, true);
  }

  // Encode the nets.
  for (const auto *net : nets) {
    encoder.encode(*net, 0);
  }

  // Compare the outputs.
  Context::Clause existsDiff;
  for (const auto &[lhsGateId, rhsGateId] : obind) {
    const auto y  = encoder.newVar();
    const auto x1 = encoder.var(lhsGateId, 0);
    const auto x2 = encoder.var(rhsGateId, 0);

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

bool Checker::areIsomorphic(const GNet &lhs,
                            const GNet &rhs,
                            const GateBindList &ibind,
                            const GateBindList &obind) const {
  // TODO:
  return false;
}

void Checker::error(Context &context,
                    const GateBindList &ibind,
                    const GateBindList &obind) const {
  bool comma;
  context.dump("miter.cnf");

  comma = false;
  std::cout << "Inputs: ";
  for (const auto &[lhsGateId, rhsGateId] : ibind) {
    if (comma) std::cout << ", ";
    comma = true;

    std::cout << context.value(context.var(lhsGateId, 0)) << "|";
    std::cout << context.value(context.var(rhsGateId, 0));
  }
  std::cout << std::endl;

  comma = false;
  std::cout << "Outputs: ";
  for (const auto &[lhsGateId, rhsGateId] : obind) {
    if (comma) std::cout << ", ";
    comma = true;

    std::cout << context.value(context.var(lhsGateId, 0)) << "|";
    std::cout << context.value(context.var(rhsGateId, 0));
  }
  std::cout << std::endl;
}

} // namespace eda::gate::debugger
