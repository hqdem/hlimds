//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

namespace eda::gate::debugger {

TEST(VlogLecTest, c17) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;

  EXPECT_EQ(fileLecTest("c17.v", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", rnd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", rnd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", bdd, PreBasis::MIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", def, PreBasis::MIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", rnd, PreBasis::MIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", bdd, PreBasis::XMG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", def, PreBasis::XMG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c17.v", rnd, PreBasis::XMG),
            CheckerResult::EQUAL);
}

TEST(VlogLecTest, c432) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);

  EXPECT_EQ(fileLecTest("c432.v", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", rnd, PreBasis::AIG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("c432.v", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", rnd, PreBasis::XAG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("c432.v", bdd, PreBasis::MIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", def, PreBasis::MIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", rnd, PreBasis::MIG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("c432.v", bdd, PreBasis::XMG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", def, PreBasis::XMG),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("c432.v", rnd, PreBasis::XMG),
            CheckerResult::UNKNOWN);
}

} // namespace eda::gate::debugger
