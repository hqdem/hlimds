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
                           const Gate::SignalList &lhsInputs,
                           Gate::Id lhsOutputId,
                           const GNet &rhs,
                           const Gate::SignalList &rhsInputs,
                           Gate::Id rhsOutputId) {
  using Link = Gate::Link;
  using GateBinding = Checker::GateBinding;

  Checker checker;
  GateBinding imap, omap;

  std::cout << lhs << std::endl;
  std::cout << rhs << std::endl;

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    imap.insert({Link(lhsInputs[i].node()), Link(rhsInputs[i].node())});
  }

  // Output bindings.
  omap.insert({Link(lhsOutputId), Link(rhsOutputId)});

  Checker::Hints hints;
  hints.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
  hints.targetBinding = std::make_shared<GateBinding>(std::move(omap));

  return checker.areEqual(lhs, rhs, hints); 
}

#define CHECK_EQUIV_TEST(G1, G2, expectedVerdict) \
  bool check##G1##G2##Test(unsigned N) { \
    Gate::SignalList lhsInputs; \
    Gate::Id lhsOutputId; \
    auto lhs = make##G1(N, lhsInputs, lhsOutputId); \
    \
    Gate::SignalList rhsInputs; \
    Gate::Id rhsOutputId; \
    auto rhs = make##G2(N, rhsInputs, rhsOutputId); \
    \
    return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId, \
                             *rhs, rhsInputs, rhsOutputId); \
  }\
  \
  TEST(CheckGNetTest, Check##G1##G2##SmallTest) {\
    EXPECT_EQ(check##G1##G2##Test(8), expectedVerdict);\
  } \
  \
  TEST(CheckGNetTest, Check##G1##G2##Test) {\
    EXPECT_EQ(check##G1##G2##Test(256), expectedVerdict);\
  }

CHECK_EQUIV_TEST(Nor,  Or,   false)
CHECK_EQUIV_TEST(Nor,  Nor,  true)
CHECK_EQUIV_TEST(Nor,  Orn,  false)
CHECK_EQUIV_TEST(Nor,  And,  false)
CHECK_EQUIV_TEST(Nor,  Nand, false)
CHECK_EQUIV_TEST(Nor,  Andn, true)

CHECK_EQUIV_TEST(Nand, Or,   false)
CHECK_EQUIV_TEST(Nand, Nor,  false)
CHECK_EQUIV_TEST(Nand, Orn,  true)
CHECK_EQUIV_TEST(Nand, And,  false)
CHECK_EQUIV_TEST(Nand, Nand, true)
CHECK_EQUIV_TEST(Nand, Andn, false)
