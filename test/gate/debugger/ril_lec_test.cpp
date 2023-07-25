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

// LEC test suites are based on RIL descriptions.

namespace eda::gate::debugger {

using Gate = eda::gate::model::Gate;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using GNet = eda::gate::model::GNet;
using PreBasis = eda::gate::premapper::PreBasis;

TEST(RilLecTest, sub) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("sub.ril", bdd, PreBasis::AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, PreBasis::AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, PreBasis::AIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", bdd, PreBasis::XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, PreBasis::XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, PreBasis::XAG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", bdd, PreBasis::MIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, PreBasis::MIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, PreBasis::MIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", bdd, PreBasis::XMG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, PreBasis::XMG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, PreBasis::XMG,
                          subFolder).isUnknown());
}

TEST(RilLecTest, add) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add.ril", bdd, PreBasis::AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, PreBasis::AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, PreBasis::AIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", bdd, PreBasis::XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, PreBasis::XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, PreBasis::XAG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", bdd, PreBasis::MIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, PreBasis::MIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, PreBasis::MIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", bdd, PreBasis::XMG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, PreBasis::XMG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, PreBasis::XMG,
                          subFolder).isUnknown());
}

TEST(RilLecTest, addSmall) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, PreBasis::XMG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, PreBasis::XMG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, PreBasis::XMG,
                        subFolder).equal());
}

TEST(RilLecTest, test) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril";

  EXPECT_TRUE(fileLecTest("test.ril", bdd,
            PreBasis::AIG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def,
            PreBasis::AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd,
            PreBasis::AIG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", bdd,
            PreBasis::XAG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def,
            PreBasis::XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd,
            PreBasis::XAG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", bdd,
            PreBasis::MIG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def,
            PreBasis::MIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd,
            PreBasis::MIG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", bdd,
            PreBasis::XMG, subFolder).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def,
            PreBasis::XMG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd,
            PreBasis::XMG, subFolder).isError());
}

TEST(RilLecTest, mulSmall) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, PreBasis::AIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, PreBasis::XAG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, PreBasis::MIG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, PreBasis::XMG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, PreBasis::XMG,
                        subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, PreBasis::XMG,
                        subFolder).equal());
}

TEST(RilLecTest, mul) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 100);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul.ril", rnd, PreBasis::AIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("mul.ril", rnd, PreBasis::XAG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("mul.ril", rnd, PreBasis::MIG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("mul.ril", rnd, PreBasis::XMG,
                          subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("mul.ril", bdd, PreBasis::XMG,
                          subFolder).equal());
  EXPECT_TRUE(fileLecTest("mul.ril", bdd, PreBasis::MIG,
                          subFolder).equal());
}
// TODO The test takes too long.
//TEST(RilLecTest, func) {
//  BddChecker bdd;
//  Checker def;
//  RndChecker rnd(false, 1000);
//  std::filesystem::path subFolder = "test/data/ril";
//
//  EXPECT_TRUE(fileLecTest("func.ril", bdd, PreBasis::AIG, Exts::RIL,
//                        subFolder).equal());
//  EXPECT_TRUE(fileLecTest("func.ril", def, PreBasis::AIG, Exts::RIL,
//                        subFolder).equal());
//  EXPECT_TRUE(fileLecTest("func.ril", rnd, PreBasis::AIG, Exts::RIL,
//                        subFolder).isUnknown());
//  EXPECT_TRUE(fileLecTest("func.ril", bdd, PreBasis::XAG, Exts::RIL,
//                        subFolder).equal());
//  EXPECT_TRUE(fileLecTest("func.ril", def, PreBasis::XAG, Exts::RIL,
//                        subFolder).equal());
//  EXPECT_TRUE(fileLecTest("func.ril", rnd, PreBasis::XAG, Exts::RIL,
//                        subFolder).isUnknown());
//}

} // namespace eda::gate::debugger
