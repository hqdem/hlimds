//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/parser_test.h"
#include "gate/printer/vcd.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace eda::gate::printer {

using GNet = eda::gate::model::GNet;

void VCDTest(const std::string &fileName) {
  using eda::gate::parser::Exts::VERILOG;
  GNet net = eda::gate::parser::getModel(fileName + ".v", "", VERILOG);
  net.sortTopologically();
  const std::string VCDFile = fileName + ".vcd";
  namespace fs = std::filesystem;
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const fs::path outputPath = "output/test/vcd_printer";
  fs::path file = homePath / outputPath / VCDFile;
  fs::path dir = file.parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  } else if (fs::exists(file)) {
    fs::remove(file);
  }
  std::vector<bool> inputValues;
  for (size_t i = 0; i < net.nSourceLinks(); i++) {
    inputValues.push_back(i % 2);
  }
  VCDPrinter::get().print(file.string(), net, inputValues);
  const std::string command = "gtkwave -x " + file.string();
  EXPECT_FALSE(std::system(command.c_str()));
  EXPECT_TRUE(fs::exists(file));
}

TEST(VCDPrinter, adder) {
  VCDTest("adder");
}

TEST(VCDPrinter, c17) {
  VCDTest("c17");
}

TEST(VCDPrinter, bar) {
  VCDTest("bar");
}

TEST(VCDPrinter, c1355) {
  VCDTest("c1355");
}

TEST(VCDPrinter, c1908) {
  VCDTest("c1908");
}

TEST(VCDPrinter, c3540) {
  VCDTest("c3540");
}

TEST(VCDPrinter, c432) {
  VCDTest("c432");
}

TEST(VCDPrinter, c499) {
  VCDTest("c499");
}

TEST(VCDPrinter, c6288) {
  VCDTest("c6288");
}

TEST(VCDPrinter, c880) {
  VCDTest("c880");
}

TEST(VCDPrinter, cavlc) {
  VCDTest("cavlc");
}

TEST(VCDPrinter, ctrl) {
  VCDTest("ctrl");
}

TEST(VCDPrinter, dec) {
  VCDTest("dec");
}

TEST(VCDPrinter, i2c) {
  VCDTest("i2c");
}

TEST(VCDPrinter, int2float) {
  VCDTest("int2float");
}

TEST(VCDPrinter, max) {
  VCDTest("max");
}

TEST(VCDPrinter, router) {
  VCDTest("router");
}

TEST(VCDPrinter, sin) {
  VCDTest("sin");
}

} // namespace eda::gate::printer
