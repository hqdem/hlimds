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

bool checkNetlistDeMorganTest() {
  const unsigned N = 1024;
  std::vector<std::pair<const Gate*, const Gate*>> imap, omap;

  // ~(x1 | ... | xN).
  Netlist lhs;
  Signal::List lhsInputs;
  for (unsigned i = 0; i < N; i++) {
    const unsigned source = lhs.add_gate();
    lhsInputs.push_back(lhs.always(source));
  }
  const unsigned lhsOutput = lhs.add_gate(GateSymbol::NOR, lhsInputs);

  // (~x1 & ... & ~xN).
  Netlist rhs;
  Signal::List rhsInputs;
  Signal::List andInputs;
  for (unsigned i = 0; i < N; i++) {
    const unsigned source = rhs.add_gate();
    const Signal signal = rhs.always(source);
    rhsInputs.push_back(signal);

    const unsigned invertor = rhs.add_gate(GateSymbol::NOR, { signal });
    andInputs.push_back(rhs.always(invertor));
  }
  const unsigned rhsOutput = rhs.add_gate(GateSymbol::AND, andInputs);

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    std::pair<const Gate*, const Gate*> binding(lhsInputs[i].gate(), rhsInputs[i].gate());
    imap.push_back(binding);
  }

  // Output bindings.
  omap.push_back({ lhs.gate(lhsOutput), rhs.gate(rhsOutput) });

  Checker checker;
  return checker.equiv(lhs, rhs, imap, omap); 
}

TEST(CheckNetlistTest, CheckNetlistDeMorganTest) {
  EXPECT_TRUE(checkNetlistDeMorganTest);
}
