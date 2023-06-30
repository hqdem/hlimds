//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/ril_lec_test.h"

using namespace eda::gate::model;
using namespace eda::rtl::compiler;
using namespace eda::rtl::parser::ril;

namespace eda::gate::debugger {

CheckerResult rilEquivalenceTest(const std::string &outSubPath,
                                 const std::string &fileName,
                                 BaseChecker &checker,
                                 PreBasis basis) {
  
  std::filesystem::path basePath = std::getenv("UTOPIA_HOME"); 
  std::filesystem::path fullPath = basePath / outSubPath / fileName;

  auto model = parse(fullPath);
  Compiler compiler(FLibraryDefault::get());
  GNet compiledNet = *compiler.compile(*model);

  compiledNet.sortTopologically();
  std::shared_ptr<GNet> initialNet = std::make_shared<GNet>(compiledNet);

  GateIdMap gatesMap;
  std::shared_ptr<GNet> premappedNet = premap(initialNet, gatesMap, basis);
  return checker.equivalent(*initialNet, *premappedNet, gatesMap);
}

TEST(RilEquivalenceTest, sub) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", rnd, PreBasis::AIG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "sub.ril", rnd, PreBasis::XAG),
            CheckerResult::UNKNOWN);
}

TEST(RilEquivalenceTest, add) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", rnd, PreBasis::AIG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add.ril", rnd, PreBasis::XAG),
            CheckerResult::UNKNOWN);
}

TEST(RilEquivalenceTest, add_small) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", rnd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "add_small.ril", rnd, PreBasis::XAG),
            CheckerResult::EQUAL);
}

TEST(RilEquivalenceTest, test) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 1000);

  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", bdd,
            PreBasis::AIG), CheckerResult::ERROR);
  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", def,
            PreBasis::AIG), CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", rnd,
            PreBasis::AIG), CheckerResult::ERROR);
  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", bdd,
            PreBasis::XAG), CheckerResult::ERROR);
  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", def,
            PreBasis::XAG), CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "test.ril", rnd,
            PreBasis::XAG), CheckerResult::ERROR);
}

TEST(RilEquivalenceTest, mul_small) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd;
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", bdd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", def, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", rnd, PreBasis::AIG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", bdd, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", def, PreBasis::XAG),
            CheckerResult::EQUAL);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul_small.ril", rnd, PreBasis::XAG),
            CheckerResult::EQUAL);
}

TEST(RilEquivalenceTest, mul) {
  BddChecker bdd;
  Checker def;
  RndChecker rnd(false, 100);
  std::filesystem::path subFolder = "test/data/ril/ril_arithmetic_tests";

  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul.ril", rnd, PreBasis::AIG),
            CheckerResult::UNKNOWN);
  EXPECT_EQ(rilEquivalenceTest(subFolder, "mul.ril", rnd, PreBasis::XAG),
            CheckerResult::UNKNOWN);
}
// TODO The test takes too long.
//TEST(RilEquivalenceTest, func) {
//  BddChecker bdd;
//  Checker def;
//  RndChecker rnd(false, 1000);
//
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", bdd,
//                                 PreBasis::AIG), CheckerResult::EQUAL);
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", def,
//                                 PreBasis::AIG), CheckerResult::EQUAL);
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", rnd,
//                                 PreBasis::AIG), CheckerResult::UNKNOWN);
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", bdd,
//                                 PreBasis::XAG), CheckerResult::EQUAL);
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", def,
//                                 PreBasis::XAG), CheckerResult::EQUAL);
//  EXPECT_EQ(rilEquivalenceTest("test/data/ril", "func.ril", rnd,
//                                 PreBasis::XAG), CheckerResult::UNKNOWN);
//}

} // namespace eda::gate::debugger
