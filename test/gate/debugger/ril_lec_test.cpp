//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

// LEC test suites are based on RIL descriptions.

namespace eda::gate::debugger {

using Gate = eda::gate::model::Gate;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using GNet = eda::gate::model::GNet;
using PreBasis = eda::gate::premapper::PreBasis;
using PreBasis::AIG;
using PreBasis::MIG;
using PreBasis::XAG;
using PreBasis::XMG;

TEST(RilLecTest, sub) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("sub.ril", bdd, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, AIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("sub.ril", bdd, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, XAG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("sub.ril", bdd, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, MIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("sub.ril", bdd, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", def, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", rnd, XMG, subPath).isUnknown());
}

TEST(RilLecTest, add) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add.ril", bdd, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, AIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("add.ril", bdd, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, XAG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("add.ril", bdd, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, MIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("add.ril", bdd, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", def, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", rnd, XMG, subPath).isUnknown());
}

TEST(RilLecTest, addSmall) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", bdd, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", def, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", rnd, XMG, subPath).equal());
}

TEST(RilLecTest, test) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subPath = "test/data/ril";

  EXPECT_TRUE(fileLecTest("test.ril", bdd, AIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd, AIG, subPath).isError());

  EXPECT_TRUE(fileLecTest("test.ril", bdd, XAG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd, XAG, subPath).isError());

  EXPECT_TRUE(fileLecTest("test.ril", bdd, MIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd, MIG, subPath).isError());

  EXPECT_TRUE(fileLecTest("test.ril", bdd, XMG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", def, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("test.ril", rnd, XMG, subPath).isError());
}

TEST(RilLecTest, mulSmall) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", bdd, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", def, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", rnd, XMG, subPath).equal());
}

TEST(RilLecTest, mul) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 100);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul.ril", rnd, AIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", rnd, XAG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", bdd, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul.ril", rnd, MIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", bdd, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul.ril", rnd, XMG, subPath).isUnknown());
}

// TODO The test takes too long.
/* TEST(RilLecTest, func) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril";

  EXPECT_TRUE(fileLecTest("func.ril", bdd, AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", def, AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", rnd, AIG, subFolder).isUnknown());

  EXPECT_TRUE(fileLecTest("func.ril", bdd, XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", def, XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", rnd, XAG, subFolder).isUnknown());
} */
} // namespace eda::gate::debugger
