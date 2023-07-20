//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

using namespace eda::gate::model;
using namespace eda::rtl::compiler;
using namespace eda::rtl::parser::ril;

namespace eda::gate::debugger {

TEST(RilEquivalenceTest, sub) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(fileLecTest("sub.ril", bdd, PreBasis::AIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", def, PreBasis::AIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", rnd, PreBasis::AIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("sub.ril", bdd, PreBasis::XAG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", def, PreBasis::XAG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", rnd, PreBasis::XAG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("sub.ril", bdd, PreBasis::MIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", def, PreBasis::MIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", rnd, PreBasis::MIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("sub.ril", bdd, PreBasis::XMG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", def, PreBasis::XMG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("sub.ril", rnd, PreBasis::XMG, subFolder),
            CheckerResult::UNKNOWN);
}

TEST(RilEquivalenceTest, add) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(fileLecTest("add.ril", bdd, PreBasis::AIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", def, PreBasis::AIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", rnd, PreBasis::AIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("add.ril", bdd, PreBasis::XAG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", def, PreBasis::XAG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", rnd, PreBasis::XAG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("add.ril", bdd, PreBasis::MIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", def, PreBasis::MIG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", rnd, PreBasis::MIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("add.ril", bdd, PreBasis::XMG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", def, PreBasis::XMG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add.ril", rnd, PreBasis::XMG, subFolder),
            CheckerResult::UNKNOWN);
}

TEST(RilEquivalenceTest, add_small) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";
  
  EXPECT_EQ(fileLecTest("add_small.ril", bdd, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", def, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", rnd, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", bdd, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", def, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", rnd, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", bdd, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", def, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", rnd, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", bdd, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", def, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("add_small.ril", rnd, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
}

TEST(RilEquivalenceTest, test) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);

  EXPECT_EQ(fileLecTest("test.ril", bdd,
            PreBasis::AIG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", def,
            PreBasis::AIG, "test/data/ril"), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("test.ril", rnd,
            PreBasis::AIG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", bdd,
            PreBasis::XAG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", def,
            PreBasis::XAG, "test/data/ril"), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("test.ril", rnd,
            PreBasis::XAG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", bdd,
            PreBasis::MIG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", def,
            PreBasis::MIG, "test/data/ril"), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("test.ril", rnd,
            PreBasis::MIG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", bdd,
            PreBasis::XMG, "test/data/ril"), CheckerResult::ERROR);
  EXPECT_EQ(fileLecTest("test.ril", def,
            PreBasis::XMG, "test/data/ril"), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("test.ril", rnd,
            PreBasis::XMG, "test/data/ril"), CheckerResult::ERROR);
}

TEST(RilEquivalenceTest, mul_small) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(fileLecTest("mul_small.ril", bdd, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", def, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", rnd, PreBasis::AIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", bdd, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", def, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", rnd, PreBasis::XAG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", bdd, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", def, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", rnd, PreBasis::MIG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", bdd, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", def, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul_small.ril", rnd, PreBasis::XMG,
                        subFolder), CheckerResult::EQUAL);
}

TEST(RilEquivalenceTest, mul) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 100);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(fileLecTest("mul.ril", rnd, PreBasis::AIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("mul.ril", rnd, PreBasis::XAG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("mul.ril", rnd, PreBasis::MIG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("mul.ril", rnd, PreBasis::XMG, subFolder),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(fileLecTest("mul.ril", bdd, PreBasis::XMG, subFolder),
            CheckerResult::EQUAL);
  EXPECT_EQ(fileLecTest("mul.ril", bdd, PreBasis::MIG, subFolder),
            CheckerResult::EQUAL);
}
// TODO The test takes too long.
//TEST(RilEquivalenceTest, func) {
//  BddChecker bdd;
//  Checker def;
//  RndChecker rnd(false, 1000);
//
//  EXPECT_EQ(fileLecTest("func.ril", bdd, PreBasis::AIG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::EQUAL);
//  EXPECT_EQ(fileLecTest("func.ril", def, PreBasis::AIG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::EQUAL);
//  EXPECT_EQ(fileLecTest("func.ril", rnd, PreBasis::AIG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::UNKNOWN);
//  EXPECT_EQ(fileLecTest("func.ril", bdd, PreBasis::XAG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::EQUAL);
//  EXPECT_EQ(fileLecTest("func.ril", def, PreBasis::XAG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::EQUAL);
//  EXPECT_EQ(fileLecTest("func.ril", rnd, PreBasis::XAG, Exts::RIL,
//                        "test/data/ril"), CheckerResult::UNKNOWN);
//}

} // namespace eda::gate::debugger
