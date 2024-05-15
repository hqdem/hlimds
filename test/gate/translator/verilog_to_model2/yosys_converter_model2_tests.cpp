 //===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/model2.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace eda::gate::model {

static constexpr const char *inPathVerilog = "test/data/gate/parser/verilog";
static constexpr const char *pathFir = "test/data/gate/verilog_to_fir";

bool VerilogTranslatorModel2Test(
    const std::string &inFileName, bool isIscas = true, bool isPicorv = false) {
  fs::path inputFullPath = eda::env::getHomePath();
  inputFullPath /= isIscas ? inPathVerilog : pathFir;
  const fs::path inputFullName = inputFullPath / inFileName;
  YosysToModel2Config cfg;
  cfg.debugMode = false;
  cfg.topModule = isPicorv ? "picorv32" : "";
  cfg.files = { inputFullName.c_str() };
  return !translateVerilogToModel2(cfg);
}

bool VerilogTranslatorModel2StandardTest() {
  return VerilogTranslatorModel2Test("test_04_05_00_1.v", false) &&
         VerilogTranslatorModel2Test("test_04_05_00_2.v", false) &&
         VerilogTranslatorModel2Test("test_04_05_00_3.v", false) &&
         VerilogTranslatorModel2Test("test_06_01_02_2.v", false) &&
         VerilogTranslatorModel2Test("test_12_01_02_1.v", false) &&
         VerilogTranslatorModel2Test("test_12_01_02_2.v", false) &&
         VerilogTranslatorModel2Test("test_12_04_01_2.v", false) &&
         VerilogTranslatorModel2Test("test_12_04_01_3.v", false) &&
         VerilogTranslatorModel2Test("test_12_04_01_4.v", false) &&
         VerilogTranslatorModel2Test("test_12_04_02_3.v", false) &&
         VerilogTranslatorModel2Test("test_14_02_04_2_1.v", false);
}

TEST(VerilogTranslatorModel2, PicoRV32) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("picorv.v", false, true));
}

TEST(VerilogTranslatorModel2, Standard) {
  EXPECT_TRUE(VerilogTranslatorModel2StandardTest());
}

TEST(VerilogTranslatorModel2, ISCAS_C17) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c17.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Adder) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("adder.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Arbiter) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("arbiter.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Bar) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("bar.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C432) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c432.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C499) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c499.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C880) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c880.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C1355) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c1355.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C1908) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c1908.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C2670) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c2670.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C3540) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c3540.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C5315) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c5315.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C6288) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c6288.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_C7552) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("c7552.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Cavlc) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("cavlc.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Ctrl) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("ctrl.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Dec) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("dec.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_i2c) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("i2c.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_int2float) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("int2float.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_log2) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("log2.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_max) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("max.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_multiplier) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("multiplier.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Priority) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("Priority.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_router) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("router.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_sin) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("sin.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_sqrt) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("sqrt.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_Square) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("square.v"));
}

TEST(VerilogTranslatorModel2, ISCAS_voter) {
  EXPECT_TRUE(VerilogTranslatorModel2Test("voter.v"));
}

} // namespace eda::gate::model
