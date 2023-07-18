//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/parser/gate_verilog_parser.h"
#include "gate/printer/verilog/gate_verilog_printer.h"

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

void printerTest(const std::string &filename, const GNet &net) {
  namespace fs = std::filesystem;
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const fs::path outputPath = "output/test/gate_verilog_printer";
  size_t extIndex = filename.find_last_of(".");
  std::string noExtName = filename.substr(0, extIndex);
  fs::path file = homePath / outputPath / noExtName;
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

GNet *parse(const std::string &infile) {
  const std::filesystem::path subCatalog = "test/data/gate/parser";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPath = homePath / subCatalog;
  const std::filesystem::path prefixPathIn = prefixPath / "verilog";

  std::string filename = prefixPathIn / infile;

  lorina::text_diagnostics consumer;
  lorina::diagnostic_engine diag(&consumer);

  GateVerilogParser parser(infile);

  lorina::return_code result = read_verilog(filename, parser, &diag);
  EXPECT_EQ(result, lorina::return_code::success);

  return parser.getGnet();
}

void printerParserTest(const std::string designName) {
  const auto *net = parse(designName);
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
  printerParserTest("adder.v");
}

TEST(GateVerilogPrinter, c17) {
  printerParserTest("c17.v");
}

TEST(GateVerilogPrinter, arbiter) {
  printerParserTest("arbiter.v");
}

TEST(GateVerilogPrinter, bar) {
  printerParserTest("bar.v");
}

TEST(GateVerilogPrinter, c1355) {
  printerParserTest("c1355.v");
}

TEST(GateVerilogPrinter, c1908) {
  printerParserTest("c1908.v");
}

TEST(GateVerilogPrinter, c3540) {
  printerParserTest("c3540.v");
}

TEST(GateVerilogPrinter, c432) {
  printerParserTest("c432.v");
}

TEST(GateVerilogPrinter, c499) {
  printerParserTest("c499.v");
}

TEST(GateVerilogPrinter, c6288) {
  printerParserTest("c6288.v");
}

TEST(GateVerilogPrinter, c880) {
  printerParserTest("c880.v");
}

TEST(GateVerilogPrinter, cavlc) {
  printerParserTest("cavlc.v");
}

TEST(GateVerilogPrinter, ctrl) {
  printerParserTest("ctrl.v");
}

TEST(GateVerilogPrinter, dec) {
  printerParserTest("dec.v");
}

TEST(GateVerilogPrinter, div) {
  printerParserTest("div.v");
}

TEST(GateVerilogPrinter, i2c) {
  printerParserTest("i2c.v");
}

TEST(GateVerilogPrinter, int2float) {
  printerParserTest("int2float.v");
}

TEST(GateVerilogPrinter, log2) {
  printerParserTest("log2.v");
}

TEST(GateVerilogPrinter, max) {
  printerParserTest("max.v");
}

TEST(GateVerilogPrinter, multiplier) {
  printerParserTest("multiplier.v");
}

TEST(GateVerilogPrinter, router) {
  printerParserTest("router.v");
}

TEST(GateVerilogPrinter, sin) {
  printerParserTest("sin.v");
}

TEST(GateVerilogPrinter, sqrt) {
  printerParserTest("sqrt.v");
}

TEST(GateVerilogPrinter, square) {
  printerParserTest("square.v");
}

TEST(GateVerilogPrinter, voter) {
  printerParserTest("voter.v");
}

} // namespace eda::gate::printer
