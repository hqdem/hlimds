//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

// LEC test suites are based on RIL descriptions.

namespace eda::gate::debugger {

using Gate = eda::gate::model::Gate;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using GNet = eda::gate::model::GNet;
using PreBasis = eda::gate::premapper::PreBasis;
using options::BDD;
using options::FRAIG;
using options::RND;
using options::SAT;
using PreBasis::AIG;
using PreBasis::MIG;
using PreBasis::XAG;
using PreBasis::XMG;

TEST(RilLecTest, sub) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(getChecker(RND)).setTries(1000);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("sub.ril", BDD, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", FRAIG, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", RND, AIG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", SAT, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("sub.ril", BDD, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", FRAIG, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", RND, XAG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", SAT, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("sub.ril", BDD, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", RND, MIG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", SAT, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("sub.ril", BDD, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("sub.ril", RND, XMG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("sub.ril", SAT, XMG, subPath).equal());
}

TEST(RilLecTest, add) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(getChecker(RND)).setTries(1000);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add.ril", BDD, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", FRAIG, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", RND, AIG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", SAT, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add.ril", BDD, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", FRAIG, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", RND, XAG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", SAT, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add.ril", BDD, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", RND, MIG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", SAT, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add.ril", BDD, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add.ril", RND, XMG, subPath).isUnknown());
  EXPECT_TRUE(fileLecTest("add.ril", SAT, XMG, subPath).equal());
}

TEST(RilLecTest, addSmall) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(true);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("add_small.ril", BDD, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", FRAIG, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", RND, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", SAT, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", BDD, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", FRAIG, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", RND, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", SAT, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", BDD, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", RND, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", SAT, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("add_small.ril", BDD, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", RND, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("add_small.ril", SAT, XMG, subPath).equal());
}

TEST(RilLecTest, test) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(getChecker(RND)).setTries(1000);
  std::filesystem::path subPath = "test/data/ril";

  EXPECT_TRUE(fileLecTest("test.ril", BDD, AIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", RND, AIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", SAT, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("test.ril", BDD, XAG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", RND, XAG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", SAT, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("test.ril", BDD, MIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", RND, MIG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", SAT, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("test.ril", BDD, XMG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", RND, XMG, subPath).isError());
  EXPECT_TRUE(fileLecTest("test.ril", SAT, XMG, subPath).equal());
}

TEST(RilLecTest, mulSmall) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(true);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul_small.ril", BDD, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", RND, AIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", SAT, AIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", BDD, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", RND, XAG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", SAT, XAG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", BDD, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", RND, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", SAT, MIG, subPath).equal());

  EXPECT_TRUE(fileLecTest("mul_small.ril", BDD, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", RND, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul_small.ril", SAT, XMG, subPath).equal());
}

TEST(RilLecTest, mul) {
  static_cast<RndChecker&>(getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(getChecker(RND)).setTries(1000);
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(fileLecTest("mul.ril", RND, AIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", RND, XAG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", BDD, MIG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul.ril", RND, MIG, subPath).isUnknown());

  EXPECT_TRUE(fileLecTest("mul.ril", BDD, XMG, subPath).equal());
  EXPECT_TRUE(fileLecTest("mul.ril", RND, XMG, subPath).isUnknown());
}

TEST(RilLecTest, unequal) {
  std::filesystem::path subPath = "test/data/ril/ril_arithmetic_tests";

  EXPECT_TRUE(twoFilesLecTest("mul_small.ril", "add6.ril",
                              FRAIG, subPath, subPath).notEqual());
  EXPECT_TRUE(twoFilesLecTest("mul_small.ril", "sub6.ril",
                              FRAIG, subPath, subPath).notEqual());
  EXPECT_TRUE(twoFilesLecTest("sub6.ril", "add6.ril",
                              FRAIG, subPath, subPath).notEqual());
}

// TODO The test takes too long.
/* TEST(RilLecTest, func) {
  std::filesystem::path subFolder = "test/data/ril";

  EXPECT_TRUE(fileLecTest("func.ril", BDD, AIG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", RND, AIG, subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("func.ril", SAT, AIG, subFolder).equal());

  EXPECT_TRUE(fileLecTest("func.ril", BDD, XAG, subFolder).equal());
  EXPECT_TRUE(fileLecTest("func.ril", RND, XAG, subFolder).isUnknown());
  EXPECT_TRUE(fileLecTest("func.ril", SAT, XAG, subFolder).equal());
} */
} // namespace eda::gate::debugger
