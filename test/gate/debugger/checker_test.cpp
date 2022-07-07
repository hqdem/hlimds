//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/checker.h"
#include "gate/model/gnet_test.h"

#include "gtest/gtest.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;

static bool checkEquivTest(unsigned N,
                           const GNet &lhs,
                           const Signal::List &lhsInputs,
                           Gate::Id lhsOutputId,
                           const GNet &rhs,
                           const Signal::List &rhsInputs,
                           Gate::Id rhsOutputId) {
  using Link = Gate::Link;
  using GateBinding = Checker::GateBinding;

  Checker checker;
  GateBinding imap, omap;

  std::cout << lhs << std::endl;
  std::cout << rhs << std::endl;

  std::cout << lhs.nSourceLinks() << ", "
            << lhs.nTargetLinks() << std::endl;
  std::cout << rhs.nSourceLinks() << ", "
            << rhs.nTargetLinks() << std::endl;

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    imap.insert({Link(lhsInputs[i]), Link(rhsInputs[i])});
  }

  // Output bindings.
  omap.insert({Link(lhsOutputId), Link(rhsOutputId)});

  Checker::Hints hints;
  hints.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
  hints.targetBinding = std::make_shared<GateBinding>(std::move(omap));

  return checker.areEqual(lhs, rhs, hints); 
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
