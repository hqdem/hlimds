//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "gate/checker/checker.h"

using namespace eda::gate::checker;
using namespace eda::gate::model;

bool checkDeMorganTest() {
  const unsigned N = 1024;
  std::vector<std::pair<unsigned, unsigned>> imap, omap;

  // ~(x1 | ... | xN).
  Netlist lhs;
  Signal::List lhsInputs;

  for (unsigned i = 0; i < N; i++) {
    const unsigned lhsInputId = lhs.add_gate();
    const Signal lhsInput = lhs.always(lhsInputId);
    lhsInputs.push_back(lhsInput);
  }

  const unsigned lhsOutputId = lhs.add_gate(GateSymbol::NOR, lhsInputs);

  // (~x1 & ... & ~xN).
  Netlist rhs;
  Signal::List rhsInputs, andInputs;

  for (unsigned i = 0; i < N; i++) {
    const unsigned rhsInputId = rhs.add_gate();
    const Signal rhsInput = rhs.always(rhsInputId);
    rhsInputs.push_back(rhsInput);

    const unsigned notGateId = rhs.add_gate(GateSymbol::NOT, { rhsInput });
    const Signal andInput = rhs.always(notGateId);
    andInputs.push_back(andInput);
  }

  const unsigned rhsOutputId = rhs.add_gate(GateSymbol::AND, andInputs);

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    std::pair<unsigned, unsigned> binding(lhsInputs[i].gate()->id(),
                                          rhsInputs[i].gate()->id());

    imap.push_back(binding);
  }

  // Output bindings.
  omap.push_back({ lhsOutputId, rhsOutputId });

  Checker checker;
  return checker.equiv(lhs, rhs, imap, omap); 
}

TEST(CheckNetlistTest, CheckDeMorganTest) {
  EXPECT_TRUE(checkDeMorganTest);
}
