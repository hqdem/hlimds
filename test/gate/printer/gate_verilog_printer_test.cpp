//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/printer/gate_verilog.h"

#include <gtest/gtest.h>
#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <cstdlib>
#include <filesystem>

using namespace eda::gate::parser::verilog;

namespace eda::gate::printer {

using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;

const std::string IVERILOG = "iverilog";

std::shared_ptr<GNet> getNet(std::function
    <std::shared_ptr<GNet>(unsigned, Gate::SignalList&, Gate::Id&)> generator) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = generator(10, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
  return net;
}

int compileVerilog(const std::string &filename) {
  const std::string command = IVERILOG + " " + filename;
  return std::system(command.c_str());
}

void printerTest(std::function
    <std::shared_ptr<GNet>(unsigned, Gate::SignalList&, Gate::Id&)> generator) {
  GateVerilogPrinter::get().print(std::cout, *getNet(generator));
}

void printerTest(const std::string &fileName, const GNet &net) {
  namespace fs = std::filesystem;
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const fs::path outputPath = "output/test/gate_verilog_printer";
  fs::path file = homePath / outputPath / fileName;
  fs::path dir = file.parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  } else if (fs::exists(file)) {
    fs::remove(file);
  }
  GateVerilogPrinter::get().print(file.string(), net);
  EXPECT_TRUE(fs::exists(file));
  EXPECT_EQ(compileVerilog(file.string()), 0);
}

void printerTest(const std::string &filename, std::function
    <std::shared_ptr<GNet>(unsigned, Gate::SignalList&, Gate::Id&)> generator) {
  printerTest(filename, *getNet(generator));
}

void printerParserTest(const std::string designName) {
  GNet *net = eda::gate::parser::parseVerilogTest(designName + ".v");
  printerTest(designName + "_gate.v", *net);
  delete net;
}

TEST(GateVerilogPrinter, OrCoutTest) {
  printerTest(eda::gate::model::makeOr);
}

TEST(GateVerilogPrinter, OrFileTest) {
  printerTest("or_gate.v", eda::gate::model::makeOr);
}

TEST(GateVerilogPrinter, MajCoutTest) {
  printerTest(eda::gate::model::makeMaj);
}

TEST(GateVerilogPrinter, MajFileTest) {
  printerTest("maj_gate.v", eda::gate::model::makeMaj);
}

TEST(GateVerilogPrinter, adder) {
  printerParserTest("adder");
}

TEST(GateVerilogPrinter, c17) {
  printerParserTest("c17");
}

TEST(GateVerilogPrinter, arbiter) {
  printerParserTest("arbiter");
}

TEST(GateVerilogPrinter, bar) {
  printerParserTest("bar");
}

TEST(GateVerilogPrinter, c1355) {
  printerParserTest("c1355");
}

TEST(GateVerilogPrinter, c1908) {
  printerParserTest("c1908");
}

TEST(GateVerilogPrinter, c3540) {
  printerParserTest("c3540");
}

TEST(GateVerilogPrinter, c432) {
  printerParserTest("c432");
}

TEST(GateVerilogPrinter, c499) {
  printerParserTest("c499");
}

TEST(GateVerilogPrinter, c6288) {
  printerParserTest("c6288");
}

TEST(GateVerilogPrinter, c880) {
  printerParserTest("c880");
}

TEST(GateVerilogPrinter, cavlc) {
  printerParserTest("cavlc");
}

TEST(GateVerilogPrinter, ctrl) {
  printerParserTest("ctrl");
}

TEST(GateVerilogPrinter, dec) {
  printerParserTest("dec");
}

TEST(GateVerilogPrinter, div) {
  printerParserTest("div");
}

TEST(GateVerilogPrinter, i2c) {
  printerParserTest("i2c");
}

TEST(GateVerilogPrinter, int2float) {
  printerParserTest("int2float");
}

TEST(GateVerilogPrinter, log2) {
  printerParserTest("log2");
}

TEST(GateVerilogPrinter, max) {
  printerParserTest("max");
}

TEST(GateVerilogPrinter, multiplier) {
  printerParserTest("multiplier");
}

TEST(GateVerilogPrinter, router) {
  printerParserTest("router");
}

TEST(GateVerilogPrinter, sin) {
  printerParserTest("sin");
}

TEST(GateVerilogPrinter, sqrt) {
  printerParserTest("sqrt");
}

TEST(GateVerilogPrinter, square) {
  printerParserTest("square");
}

TEST(GateVerilogPrinter, voter) {
  printerParserTest("voter");
}

} // namespace eda::gate::printer
