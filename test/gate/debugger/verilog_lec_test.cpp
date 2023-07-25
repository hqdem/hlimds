//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

// LEC test suites are based on gate-level Verilog descriptions.

namespace eda::gate::debugger {

TEST(VlogLecTest, c17) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;

  EXPECT_TRUE(fileLecTest("c17.v", bdd, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", def, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", rnd, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", bdd, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", def, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", rnd, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", bdd, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", def, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", rnd, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", bdd, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", def, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", rnd, PreBasis::XMG).equal());
}

TEST(VlogLecTest, c432) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);

  EXPECT_TRUE(fileLecTest("c432.v", bdd, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", def, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", rnd, PreBasis::AIG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", bdd, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", def, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", rnd, PreBasis::XAG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", bdd, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", def, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", rnd, PreBasis::MIG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", bdd, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", def, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", rnd, PreBasis::XMG).isUnknown());
}

} // namespace eda::gate::debugger
