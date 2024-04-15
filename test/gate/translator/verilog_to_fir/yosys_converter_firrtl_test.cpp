 //===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/firrtl.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace eda::gate::model {

static constexpr const char *pathFir = "test/data/gate/verilog_to_fir";
static constexpr const char *inPathVerilog = "test/data/gate/parser/verilog";

static std::string replaceFileExtension(
    const std::string &filename, const std::string &newExtension) {
  size_t dotPos = filename.find_last_of('.');
  if (dotPos != std::string::npos) {
    return filename.substr(0, dotPos) + '.' + newExtension;
  } else {
    return filename + '.' + newExtension;
  }
}

static bool compareStringstreams(
    std::stringstream &ss1, std::stringstream &ss2) {
  std::vector<std::string> vec1;
  std::vector<std::string> vec2;
  std::string line;
  while (std::getline(ss1, line)) {
    vec1.push_back(line);
  }
  while (std::getline(ss2, line)) {
    vec2.push_back(line);
  }
  std::sort(vec1.begin(), vec1.end());
  std::sort(vec2.begin(), vec2.end());
  if (vec1.size() != vec2.size()) {
    return false;
  }
  for (size_t i = 0; i < vec1.size(); ++i) {
    if (vec1[i] != vec2[i]) {
      return false;
    }
  }
  return true;
}

bool compareOutputWithFile(
    const std::string &filePathVerilog, const std::string &filePathFir) {
  std::stringstream buffer;
  std::streambuf* oldCoutBuffer = std::cout.rdbuf(buffer.rdbuf());
  FirrtlConfig cfg;
  cfg.debugMode = false;
  cfg.files = { filePathVerilog };
  translateToFirrtl(cfg);
  std::cout.rdbuf(oldCoutBuffer);
  std::ifstream file(filePathFir);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePathFir << std::endl;
    return false;
  }
  std::stringstream fileContent;
  fileContent << file.rdbuf();
  return compareStringstreams(buffer,fileContent);
}

bool YosysConverterFirrtlTest(
    const std::string &inFileName, bool isPicorvOrStandard = false) {
  const std::string &homePath = std::string(getenv("UTOPIA_HOME"));
  fs::path inputFullPath = homePath;
  fs::path pathFirrtlCompare = homePath;
  inputFullPath /= isPicorvOrStandard ? pathFir : inPathVerilog;
  pathFirrtlCompare /= pathFir;
  std::string compareFilename = replaceFileExtension(inFileName, "fir");
  const fs::path inputFullName = inputFullPath / inFileName;
  const fs::path CompareFullName = pathFirrtlCompare / compareFilename;
  return compareOutputWithFile(inputFullName.string(), CompareFullName.string());
}

bool YosysConverterFirrtlTest1() {
return YosysConverterFirrtlTest("test_04_05_00_1.v", true) &&
       YosysConverterFirrtlTest("test_06_01_02_2.v", true) &&
       YosysConverterFirrtlTest("test_12_01_02_1.v", true) &&
       YosysConverterFirrtlTest("test_12_01_02_2.v", true) &&
       YosysConverterFirrtlTest("test_12_04_01_2.v", true) &&
       YosysConverterFirrtlTest("test_12_04_01_3.v", true) &&
       YosysConverterFirrtlTest("test_12_04_01_4.v", true) &&
       YosysConverterFirrtlTest("test_12_04_02_3.v", true) &&
       YosysConverterFirrtlTest("test_14_02_04_2_1.v", true);
}

TEST(VerilogTranslatorFirrtl, ISCAS_Adder) {
  EXPECT_TRUE(YosysConverterFirrtlTest("adder.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Arbiter) {
  EXPECT_TRUE(YosysConverterFirrtlTest("arbiter.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Bar) {
  EXPECT_TRUE(YosysConverterFirrtlTest("bar.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C17) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c17.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C432) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c432.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C499) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c499.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C880) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c880.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C1355) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c1355.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C1908) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c1908.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C2670) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c2670.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C3540) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c3540.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C5315) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c5315.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C6288) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c6288.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_C7552) {
  EXPECT_TRUE(YosysConverterFirrtlTest("c7552.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Cavlc) {
  EXPECT_TRUE(YosysConverterFirrtlTest("cavlc.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Ctrl) {
  EXPECT_TRUE(YosysConverterFirrtlTest("ctrl.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Dec) {
  EXPECT_TRUE(YosysConverterFirrtlTest("dec.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Div) {
  EXPECT_TRUE(YosysConverterFirrtlTest("div.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_i2c) {
  EXPECT_TRUE(YosysConverterFirrtlTest("i2c.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_int2float) {
  EXPECT_TRUE(YosysConverterFirrtlTest("int2float.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_log2) {
  EXPECT_TRUE(YosysConverterFirrtlTest("log2.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_max) {
  EXPECT_TRUE(YosysConverterFirrtlTest("max.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_memctrl) {
  EXPECT_TRUE(YosysConverterFirrtlTest("memctrl.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_multiplier) {
  EXPECT_TRUE(YosysConverterFirrtlTest("multiplier.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Priority) {
  EXPECT_TRUE(YosysConverterFirrtlTest("Priority.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_router) {
  EXPECT_TRUE(YosysConverterFirrtlTest("router.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_sin) {
  EXPECT_TRUE(YosysConverterFirrtlTest("sin.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_sqrt) {
  EXPECT_TRUE(YosysConverterFirrtlTest("sqrt.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_Square) {
  EXPECT_TRUE(YosysConverterFirrtlTest("square.v"));
}

TEST(VerilogTranslatorFirrtl, ISCAS_voter) {
  EXPECT_TRUE(YosysConverterFirrtlTest("voter.v"));
}

TEST(VerilogTranslatorFirrtl, PicoRV32) {
  EXPECT_TRUE(YosysConverterFirrtlTest("picorv.v", true));
}

TEST(VerilogTranslatorFirrtl, Standard) {
  EXPECT_TRUE(YosysConverterFirrtlTest1());
}
} // namespace eda::gate::model
