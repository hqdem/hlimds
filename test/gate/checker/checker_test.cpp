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

static bool checkEquivTest(unsigned N,
                           const Netlist &lhs,
                           const Signal::List &lhsInputs,
                           Gate::Id lhsOutputId,
                           const Netlist &rhs,
                           const Signal::List &rhsInputs,
                           Gate::Id rhsOutputId) {
  Checker::GateBindList imap, omap;

  std::cout << lhs << std::endl;
  std::cout << rhs << std::endl;

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    Checker::GateBind binding(lhsInputs[i].gateId(),
                              rhsInputs[i].gateId());
    imap.push_back(binding);
  }

  // Output bindings.
  omap.push_back({ lhsOutputId, rhsOutputId });

  Checker checker;
  return checker.equiv(lhs, rhs, imap, omap); 
}

bool checkNorNorTest(unsigned N) {
  // ~(x1 | ... | xN).
  Signal::List lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // ~(x1 | ... | xN).
  Signal::List rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeNor(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId);
}

bool checkNorAndnTest(unsigned N) {
  // ~(x1 | ... | xN).
  Signal::List lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // (~x1 & ... & ~xN).
  Signal::List rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeAndn(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId);
}

bool checkNorAndTest(unsigned N) {
  // ~(x1 | ... | xN).
  Signal::List lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // (x1 & ... & xN).
  Signal::List rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeAnd(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId);
}

TEST(CheckNetlistTest, CheckNorNorTest) {
  EXPECT_TRUE(checkNorNorTest(16));
}

TEST(CheckNetlistTest, CheckNorAndnTest) {
  EXPECT_TRUE(checkNorAndnTest(16));
}

TEST(CheckNetlistTest, CheckNorAndTest) {
  EXPECT_FALSE(checkNorAndTest(16));
}
