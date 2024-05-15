//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/fir/fir_model2.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace eda::gate::model {

static constexpr const char *inPath = "test/data/gate/translator/fir";
static constexpr const char *outPath = "output/test/gate/translator/fir";

bool firrtlTranslatorTest(const std::string &inFileName) {
  fs::path homePath = eda::env::getHomePath();
  fs::path inputFullPath = homePath;
  inputFullPath /= inPath;
  const fs::path inputFullName = inputFullPath / inFileName;

  fs::path outputFullPath = homePath;
  outputFullPath /= outPath;

  return printNetlist(inputFullName, outputFullPath);
}

// 'MLIR' tests.
TEST(FIRRTLTranslatorMLIR, InToOut) {
  EXPECT_TRUE(firrtlTranslatorTest("in_to_out.mlir"));
}

TEST(FIRRTLTranslatorMLIR, OutTo) {
  EXPECT_TRUE(firrtlTranslatorTest("out_to.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleMux) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_mux.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleAdd) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_add.mlir"));
}

TEST(FIRRTLTranslatorMLIR, TwoLevelAdd) {
  EXPECT_TRUE(firrtlTranslatorTest("two_level_add.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleInstance) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_instance.mlir"));
}

TEST(FIRRTLTranslatorMLIR, TwoLevelInstance) {
  EXPECT_TRUE(firrtlTranslatorTest("two_level_instance.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleXor) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_xor.mlir"));
}

TEST(FIRRTLTranslatorMLIR, TwoLevelXor) {
  EXPECT_TRUE(firrtlTranslatorTest("two_level_xor.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleRegister) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_reg.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleRegisterWithReset) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_regreset.mlir"));
}

TEST(FIRRTLTranslatorMLIR, SimpleConstant) {
  EXPECT_TRUE(firrtlTranslatorTest("simple_constant.mlir"));
}

TEST(FIRRTLTranslatorMLIR, DotProduct) {
  EXPECT_TRUE(firrtlTranslatorTest("dot_product.mlir"));
}

TEST(FIRRTLTranslatorMLIR, AddSameInputs) {
  EXPECT_TRUE(firrtlTranslatorTest("add_same_inputs.mlir"));
}

TEST(FIRRTLTranslatorMLIR, AddInstanceMix) {
  EXPECT_TRUE(firrtlTranslatorTest("add_instance_mix.mlir"));
}

} // namespace eda::gate::model
