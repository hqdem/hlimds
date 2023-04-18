//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/optimizer/examples.h"
#include "gate/printer/verilog/gate_verilog_printer.h"

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>

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
  fs::path file = homePath / filename;
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

TEST(GateVerilogPrinter, OrCoutTest) {
  printerTest(makeOr);
}

TEST(GateVerilogPrinter, OrFileTest) {
  printerTest("output/test/gate_verilog_printer/or_gate.v", makeOr);
}

TEST(GateVerilogPrinter, MajCoutTest) {
  printerTest(makeMaj);
}

TEST(GateVerilogPrinter, MajFileTest) {
  printerTest("output/test/gate_verilog_printer/maj_gate.v", makeMaj);
}

TEST(GateVerilogPrinter, Gnet1CoutTest) {
    GNet net;
    eda::gate::optimizer::gnet1(net);
    GateVerilogPrinter::get().print(std::cout, net);
}

TEST(GateVerilogPrinter, Gnet1FileTest) {
    GNet net;
    eda::gate::optimizer::gnet1(net);
    printerTest("output/test/gate_verilog_printer/gnet1.v", net);
}

} // namespace eda::gate::printer
