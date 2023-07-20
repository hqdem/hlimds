//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"
#include "gtest/gtest.h"
#include "gate/printer/graphml.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/builder.h"
#include "rtl/parser/ril/parser.h"

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <string>

using namespace lorina;

const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));

void printGNet(eda::gate::model::GNet* gnet,
    const std::string &infile,
    const std::string &outfile) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  const std::filesystem::path outDataSubCatalog = "test/data/gate/printer/graphml";
  const std::filesystem::path prefixPathOut = homePath / "build" / outDataSubCatalog;

  system(std::string("mkdir -p ").append(prefixPathOut).c_str());

  std::string outFilename = prefixPathOut / (outfile + ".graphml");
  std::ofstream printedFile;
  eda::printer::graphMl::toGraphMl graphMlTest;

  printedFile.open(outFilename);
  graphMlTest.printer(printedFile, *gnet);
  printedFile.close();
}

/* Parses input Verilog file, builds GNet representation,
 * and prints it into GraphML format.
 */
void fromVerilog(const std::string &infile, const std::string &outfile) {
  const std::filesystem::path inDataSubCatalog = "test/data/gate/parser/verilog";
  const std::filesystem::path prefixPathIn =  homePath / inDataSubCatalog;
  std::string filename = prefixPathIn / infile;

  text_diagnostics consumer;
  diagnostic_engine diag(&consumer);

  eda::gate::parser::verilog::GateVerilogParser parser(infile);
  return_code result = read_verilog(filename, parser, &diag);
  EXPECT_EQ(result, return_code::success);

  printGNet(parser.getGnet(), infile, outfile);

  delete parser.getGnet();
}

/* Parses input RIL file, builds GNet model,
 * and prints it into GraphML format.
 */
void fromRIL(const std::string &infile, const std::string &outfile) {
  const std::filesystem::path inDataSubCatalog = "test/data/ril";
  const std::filesystem::path prefixPathIn =  homePath / inDataSubCatalog;
  std::string filename = prefixPathIn / (infile + ".ril");

  auto model = eda::rtl::parser::ril::parse(filename);
  eda::rtl::compiler::Compiler compiler(eda::rtl::library::FLibraryDefault::get());
  auto gnet = compiler.compile(*model);

  printGNet(gnet.get(), infile, outfile);
}

TEST(Verilog2GraphMlTest, adder) {
  fromVerilog("adder.v", "verilog_adder");
}

TEST(Verilog2GraphMlTest, c17) {
  fromVerilog("c17.v", "verilog_c17");
}

TEST(Verilog2GraphMlTest, arbiter) {
  fromVerilog("arbiter.v", "verilog_arbiter");
}

TEST(Verilog2GraphMlTest, bar) {
  fromVerilog("bar.v", "verilog_bar");
}

TEST(Verilog2GraphMlTest, c1355) {
  fromVerilog("c1355.v", "verilog_c1355");
}

TEST(Verilog2GraphMlTest, c1908) {
  fromVerilog("c1908.v", "verilog_c1908");
}

TEST(Verilog2GraphMlTest, c3540) {
  fromVerilog("c3540.v", "verilog_c3540");
}

TEST(Verilog2GraphMlTest, c432) {
  fromVerilog("c432.v", "verilog_c432");
}

TEST(Verilog2GraphMlTest, c499) {
  fromVerilog("c499.v", "verilog_c499");
}

TEST(Verilog2GraphMlTest, c6288) {
  fromVerilog("c6288.v", "verilog_c6288");
}

TEST(Verilog2GraphMlTest, c880) {
  fromVerilog("c880.v", "verilog_c880");
}

TEST(Verilog2GraphMlTest, cavlc) {
  fromVerilog("cavlc.v", "verilog_cavlc");
}

TEST(Verilog2GraphMlTest, ctrl) {
  fromVerilog("ctrl.v", "verilog_ctrl");
}

TEST(Verilog2GraphMlTest, dec) {
  fromVerilog("dec.v", "verilog_dec");
}

TEST(Verilog2GraphMlTest, div) {
  fromVerilog("div.v", "verilog_div");
}

TEST(Verilog2GraphMlTest, i2c) {
  fromVerilog("i2c.v", "verilog_i2c");
}

TEST(Verilog2GraphMlTest, int2float) {
  fromVerilog("int2float.v", "verilog_int2float");
}

TEST(Verilog2GraphMlTest, log2) {
  fromVerilog("log2.v", "verilog_log2");
}

TEST(Verilog2GraphMlTest, max) {
  fromVerilog("max.v", "verilog_max");
}

TEST(Verilog2GraphMlTest, multiplier) {
  fromVerilog("multiplier.v", "verilog_multiplier");
}

TEST(Verilog2GraphMlTest, router) {
  fromVerilog("router.v", "verilog_router");
}

TEST(Verilog2GraphMlTest, sin) {
  fromVerilog("sin.v", "verilog_sin");
}

TEST(Verilog2GraphMlTest, sqrt) {
  fromVerilog("sqrt.v", "verilog_sqrt");
}

TEST(Verilog2GraphMlTest, square) {
  fromVerilog("square.v", "verilog_square");
}

TEST(Verilog2GraphMlTest, voter) {
  fromVerilog("voter.v", "verilog_voter");
}

// TODO: uncomment when RIL parser will be fixed
// TEST(RIL2GraphMlTest, dff) {
//   fromRIL("dff", "ril_dff");
// }

TEST(RIL2GraphMlTest, func) {
  fromRIL("func", "ril_func");
}

TEST(RIL2GraphMlTest, test) {
  fromRIL("test", "ril_test");
}

TEST(RIL2GraphMlTest, add) {
  fromRIL("ril_arithmetic_tests/add", "ril_add");
}

TEST(RIL2GraphMlTest, mul) {
  fromRIL("ril_arithmetic_tests/mul", "ril_mul");
}

TEST(RIL2GraphMlTest, sub) {
  fromRIL("ril_arithmetic_tests/sub", "ril_sub");
}
