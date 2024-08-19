//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/verilog/verilog_firrtl.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace eda::gate::translator {

static constexpr const char *inPath = "test/data/gate/translator/verilog";
static constexpr const char *outPath = "output/test/gate/translator/verilog";

bool translatorVerilogFIRRTLTest(bool debugMode,
                                 const std::string &inFileName,
                                 const std::string &topModule,
                                 const std::string &outFileName) {
  fs::path inputPath = inPath;
  const fs::path inputName = inputPath / inFileName;

  fs::path outputPath = outPath;

  FirrtlConfig firrtlConfig;
  firrtlConfig.debugMode = debugMode;
  firrtlConfig.topModule = topModule;
  firrtlConfig.files = { inputName.c_str() };
  if (!outputPath.empty()) {
    fs::create_directories(outputPath);
  }
  firrtlConfig.outputFileName = (outputPath / outFileName).c_str();

  return translateVerilogFIRRTL(firrtlConfig);
}

TEST(TranslatorVerilogFIRRTL, Mux) {
  EXPECT_TRUE(translatorVerilogFIRRTLTest(false,
                                          "mux_test.v",
                                          "mux",
                                          "mux.fir"));
}

TEST(TranslatorVerilogFIRRTL, AndOr) {
  EXPECT_TRUE(translatorVerilogFIRRTLTest(false,
                                          "andor_test.v",
                                          "andor",
                                          "andor.fir"));
}

} // namespace eda::gate::translator