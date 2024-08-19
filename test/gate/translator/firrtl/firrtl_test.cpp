//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/firrtl/firrtl_net.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace eda::gate::translator {

static constexpr const char *inPath = "test/data/gate/translator/firrtl";
static constexpr const char *outPath = "output/test/gate/translator/firrtl";

bool translatorFIRRTLTest(const std::string &inFileName) {
  fs::path homePath = eda::env::getHomePath();
  fs::path inputFullPath = homePath;
  inputFullPath /= inPath;
  const fs::path inputFullName = inputFullPath / inFileName;

  fs::path outputFullPath = homePath;
  outputFullPath /= outPath;

  return printNetlist(inputFullName, outputFullPath);
}

// 'MLIR' tests.
TEST(TranslatorFIRRTLMLIR, InToOut) {
  EXPECT_TRUE(translatorFIRRTLTest("in_to_out.mlir"));
}

TEST(TranslatorFIRRTLMLIR, OutTo) {
  EXPECT_TRUE(translatorFIRRTLTest("out_to.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleMux) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_mux.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleAdd) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_add.mlir"));
}

TEST(TranslatorFIRRTLMLIR, TwoLevelAdd) {
  EXPECT_TRUE(translatorFIRRTLTest("two_level_add.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleInstance) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_instance.mlir"));
}

TEST(TranslatorFIRRTLMLIR, TwoLevelInstance) {
  EXPECT_TRUE(translatorFIRRTLTest("two_level_instance.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleXor) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_xor.mlir"));
}

TEST(TranslatorFIRRTLMLIR, TwoLevelXor) {
  EXPECT_TRUE(translatorFIRRTLTest("two_level_xor.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleRegister) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_reg.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleRegisterWithReset) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_regreset.mlir"));
}

TEST(TranslatorFIRRTLMLIR, SimpleConstant) {
  EXPECT_TRUE(translatorFIRRTLTest("simple_constant.mlir"));
}

TEST(TranslatorFIRRTLMLIR, DotProduct) {
  EXPECT_TRUE(translatorFIRRTLTest("dot_product.mlir"));
}

TEST(TranslatorFIRRTLMLIR, AddSameInputs) {
  EXPECT_TRUE(translatorFIRRTLTest("add_same_inputs.mlir"));
}

TEST(TranslatorFIRRTLMLIR, AddInstanceMix) {
  EXPECT_TRUE(translatorFIRRTLTest("add_instance_mix.mlir"));
}

/// Square Root using Harmonized Parabolic Synthesis (Pipelined).
TEST(TranslatorFIRRTLSquareRoot, SquareRootPipelined) {
  EXPECT_TRUE(translatorFIRRTLTest("sqrt_pipeline.fir"));
}

/// Square Root using Harmonized Parabolic Synthesis (Not Pipelined).
TEST(TranslatorFIRRTLSquareRoot, SquareRootNotPipelined) {
  EXPECT_TRUE(translatorFIRRTLTest("sqrt_no_pipeline.fir"));
}

} // namespace eda::gate::translator