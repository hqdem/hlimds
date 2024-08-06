//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/fir/fir_net.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace eda::gate::translator {

static constexpr const char *inPath = "test/data/gate/translator/fir";
static constexpr const char *outPath = "output/test/gate/translator/fir";

bool translatorFIRTest(const std::string &inFileName) {
  fs::path homePath = eda::env::getHomePath();
  fs::path inputFullPath = homePath;
  inputFullPath /= inPath;
  const fs::path inputFullName = inputFullPath / inFileName;

  fs::path outputFullPath = homePath;
  outputFullPath /= outPath;

  return printNet(inputFullName, outputFullPath);
}

// 'MLIR' tests.
TEST(TranslatorFIRMLIR, InToOut) {
  EXPECT_TRUE(translatorFIRTest("in_to_out.mlir"));
}

TEST(TranslatorFIRMLIR, OutTo) {
  EXPECT_TRUE(translatorFIRTest("out_to.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleMux) {
  EXPECT_TRUE(translatorFIRTest("simple_mux.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleAdd) {
  EXPECT_TRUE(translatorFIRTest("simple_add.mlir"));
}

TEST(TranslatorFIRMLIR, TwoLevelAdd) {
  EXPECT_TRUE(translatorFIRTest("two_level_add.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleInstance) {
  EXPECT_TRUE(translatorFIRTest("simple_instance.mlir"));
}

TEST(TranslatorFIRMLIR, TwoLevelInstance) {
  EXPECT_TRUE(translatorFIRTest("two_level_instance.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleXor) {
  EXPECT_TRUE(translatorFIRTest("simple_xor.mlir"));
}

TEST(TranslatorFIRMLIR, TwoLevelXor) {
  EXPECT_TRUE(translatorFIRTest("two_level_xor.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleRegister) {
  EXPECT_TRUE(translatorFIRTest("simple_reg.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleRegisterWithReset) {
  EXPECT_TRUE(translatorFIRTest("simple_regreset.mlir"));
}

TEST(TranslatorFIRMLIR, SimpleConstant) {
  EXPECT_TRUE(translatorFIRTest("simple_constant.mlir"));
}

TEST(TranslatorFIRMLIR, DotProduct) {
  EXPECT_TRUE(translatorFIRTest("dot_product.mlir"));
}

TEST(TranslatorFIRMLIR, AddSameInputs) {
  EXPECT_TRUE(translatorFIRTest("add_same_inputs.mlir"));
}

TEST(TranslatorFIRMLIR, AddInstanceMix) {
  EXPECT_TRUE(translatorFIRTest("add_instance_mix.mlir"));
}

/// Square Root using Harmonized Parabolic Synthesis (Pipelined).
TEST(TranslatorFIRSquareRoot, SquareRootPipelined) {
  EXPECT_TRUE(translatorFIRTest("sqrt_pipeline.fir"));
}

/// Square Root using Harmonized Parabolic Synthesis (Not Pipelined).
TEST(TranslatorFIRSquareRoot, SquareRootNotPipelined) {
  EXPECT_TRUE(translatorFIRTest("sqrt_no_pipeline.fir"));
}

} // namespace eda::gate::translator