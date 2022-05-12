//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "gate/debugger/checker.h"

#include "gate/model/gnet_test.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;

static bool checkEquivTest(unsigned N,
                           const GNet &lhs,
                           const Signal::List &lhsInputs,
                           Gate::Id lhsOutputId,
                           const GNet &rhs,
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
  return checker.areEqual(lhs, rhs, imap, omap); 
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

TEST(CheckGNetTest, CheckNorNorTest) {
  EXPECT_TRUE(checkNorNorTest(16));
}

TEST(CheckGNetTest, CheckNorAndnTest) {
  EXPECT_TRUE(checkNorAndnTest(16));
}

TEST(CheckGNetTest, CheckNorAndTest) {
  EXPECT_FALSE(checkNorAndTest(16));
}
