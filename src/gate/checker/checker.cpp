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

bool Checker::equiv(const Netlist &lhs,
                    const Netlist &rhs,
                    const Checker::GateBindList &ibind,
                    const Checker::GateBindList &obind) const {
  Encoder encoder;

  const std::size_t lhsOffset = 0;
  const std::size_t rhsOffset = lhs.size();

  // Encode the netlists.
  encoder.setOffset(lhsOffset);
  encoder.encode(lhs, 0);

  encoder.setOffset(rhsOffset);
  encoder.encode(rhs, 0);

  // Equate the inputs.
  for (const auto &[lhsGateId, rhsGateId] : ibind) {
    const auto x = Context::var(lhsOffset, lhsGateId, 0);
    const auto y = Context::var(rhsOffset, rhsGateId, 0);

    encoder.encodeBuf(y, x, true);
  }

  // Compare the outputs.
  Context::Clause existsDiff(obind.size());
  for (const auto &[lhsGateId, rhsGateId] : obind) {
    const auto y  = encoder.newVar();
    const auto x1 = Context::var(lhsOffset, lhsGateId, 0);
    const auto x2 = Context::var(rhsOffset, rhsGateId, 0);

    encoder.encodeXor(y, x1, x2, true, true, true);
    existsDiff.push(Context::lit(y, true));
  }

  // (lOut[1] != rOut[1]) || ... || (lOut[m] != rOut[m]).
  encoder.encode(existsDiff);

  return !encoder.solve();
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

    if (lhsTrigger->kind() != rhsTrigger->kind())
      return false;

    imap.push_back({ lhsTrigger->id(), rhsTrigger->id() });

    assert(lhsTrigger->arity() == rhsTrigger->arity());
    for (unsigned i = 0; i < lhsTrigger->arity(); i++) {
      const Signal lhsInput = lhsTrigger->input(i);
      const Signal rhsInput = rhsTrigger->input(i);

      // TODO: Handle clocks and resets.
      omap.push_back({ lhsInput.gate()->id(), rhsInput.gate()->id() });
    }
  }

  return equiv(lhs, rhs, imap, omap);
}

} // namespace eda::gate::checker
