//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "gate/checker/checker.h"

#include "gate/model/netlist_test.h"

using namespace eda::gate::checker;
using namespace eda::gate::model;

bool checkDeMorganTest() {
  const unsigned N = 1024;
  std::vector<std::pair<unsigned, unsigned>> imap, omap;

  // ~(x1 | ... | xN).
  Signal::List lhsInputs;
  unsigned lhsOutputId;
  auto lhs = notOfOrs(N, lhsInputs, lhsOutputId);

  // (~x1 & ... & ~xN).
  Signal::List rhsInputs;
  unsigned rhsOutputId;
  auto rhs = andOfNots(N, rhsInputs, rhsOutputId);

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    std::pair<unsigned, unsigned> binding(lhsInputs[i].gate()->id(),
                                          rhsInputs[i].gate()->id());

    imap.push_back(binding);
  }

  // Output bindings.
  omap.push_back({ lhsOutputId, rhsOutputId });

  Checker checker;
  return checker.equiv(*lhs, *rhs, imap, omap); 
}

TEST(CheckNetlistTest, CheckDeMorganTest) {
  EXPECT_TRUE(checkDeMorganTest);
}
