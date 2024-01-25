//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

/// LEC test suites are based on gate-level Verilog descriptions.

namespace eda::gate::debugger {

TEST(VlogLecTest, c17) {
  static_cast<RndChecker&>(getChecker(options::RND)).setExhaustive(true);

  EXPECT_TRUE(fileLecTest("c17.v", options::BDD, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::FRAIG, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::RND, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::SAT, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::BDD, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::FRAIG, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::RND, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::SAT, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::BDD, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::RND, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::SAT, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::BDD, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::RND, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c17.v", options::SAT, PreBasis::XMG).equal());
}

TEST(VlogLecTest, c432) {
  static_cast<RndChecker&>(getChecker(options::RND)).setExhaustive(false);
  static_cast<RndChecker&>(getChecker(options::RND)).setTries(1000);

  EXPECT_TRUE(fileLecTest("c432.v", options::BDD, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::FRAIG, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::RND, PreBasis::AIG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", options::SAT, PreBasis::AIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::BDD, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::FRAIG, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::RND, PreBasis::XAG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", options::SAT, PreBasis::XAG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::BDD, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::RND, PreBasis::MIG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", options::SAT, PreBasis::MIG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::BDD, PreBasis::XMG).equal());
  EXPECT_TRUE(fileLecTest("c432.v", options::RND, PreBasis::XMG).isUnknown());
  EXPECT_TRUE(fileLecTest("c432.v", options::SAT, PreBasis::XMG).equal());
}

// TODO The test takes 20 minutes.
//TEST(VlogLecTest, unequal) {
//  EXPECT_TRUE(twoFilesLecTest("c499.v",
//                              "c1355.v", options::FRAIG).notEqual());
//}

} // namespace eda::gate::debugger
