//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/encoder/encoder.h"
#include "gate/checker/checker.h"

#include <cassert>

using namespace eda::gate::encoder;

namespace eda::gate::checker {

bool Checker::equiv(const std::vector<Netlist> &nets,
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

  // Encode the netlists.
  for (const auto &net : nets) {
    encoder.encode(net, 0);
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

bool Checker::equiv(const Netlist &lhs,
                    const Netlist &rhs,
                    const Checker::GateBindList &ibind,
                    const Checker::GateBindList &obind) const {
  return equiv({ lhs, rhs }, nullptr, ibind, obind);
}

bool Checker::equiv(const Netlist &lhs,
                    const Netlist &rhs,
                    const GateBindList &ibind,
                    const GateBindList &obind,
                    const GateBindList &tbind) const {
  GateBindList imap(ibind);
  GateBindList omap(obind);

  // Cut triggers.
  for (const auto &[lhsTriggerId, rhsTriggerId] : tbind) {
    const Gate *lhsTrigger = lhs.gate(lhsTriggerId);
    const Gate *rhsTrigger = rhs.gate(rhsTriggerId);

    if (lhsTrigger->kind() != rhsTrigger->kind()) {
      return false;
    }

    imap.push_back({ lhsTrigger->id(), rhsTrigger->id() });

    assert(lhsTrigger->arity() == rhsTrigger->arity());
    for (unsigned i = 0; i < lhsTrigger->arity(); i++) {
      const Signal lhsInput = lhsTrigger->input(i);
      const Signal rhsInput = rhsTrigger->input(i);

      omap.push_back({ lhsInput.gate()->id(), rhsInput.gate()->id() });
    }
  }

  return equiv(lhs, rhs, imap, omap);
}

bool Checker::equiv(const Netlist &lhs,
                    const Netlist &rhs,
                    const Netlist &enc,
                    const Netlist &dec,
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
    connectTo.insert({ encInId, lhs.gate(lhsTriId)->input(0).gate()->id() });
  }

  // Connect the LHS-trigger outputs to the decoder outputs.
  for (const auto &[lhsTriId, decOutId] : lhsTriDecOut) {
    connectTo.insert({ lhsTriId, decOutId });
  }

  // Append the encoder outputs and the RHS-trigger inputs to the outputs.
  for (const auto &[rhsTriId, encOutId] : rhsTriEncOut) {
    omap.push_back({ encOutId, rhs.gate(rhsTriId)->input(0).gate()->id() });
  }

  // Append the decoder inputs and the RHS-trigger outputs to to the inputs.
  for (const auto &[rhsTriId, decInId] : rhsTriDecIn) {
    imap.push_back({ decInId, rhsTriId });
  }

  return equiv({ lhs, rhs, enc, dec }, &connectTo, imap, omap);
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

} // namespace eda::gate::checker
