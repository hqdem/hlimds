//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"
#include "gate/transformer/hmetis.h"
#include "gtest/gtest.h"
#include "util/partition_hgraph.h"

#define FMT_HEADER_ONLY
#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

using namespace lorina;

void parse(const std::string &infile) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  const std::filesystem::path subCatalog = "test/data/gate/parser";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPath = homePath / subCatalog;
  const std::filesystem::path prefixPathIn = prefixPath / "input";
  const std::filesystem::path prefixPathOut = prefixPath / "output";

  std::string filename = prefixPathIn / (infile + ".v");
  std::string outFilename = prefixPathOut / (infile + ".dot");
  std::string outBaseFilename = prefixPathOut / ("base" + infile + ".dot");

  text_diagnostics consumer;
  diagnostic_engine diag(&consumer);

  GateVerilogParser parser(infile);

  return_code result = read_verilog(filename, parser, &diag);
  EXPECT_EQ(result, return_code::success);
  parser.dotPrint(outFilename);

  HMetisPrinter metis(*parser.getGnet());
  HyperGraph graph(metis.getWeights(), metis.getEptr(),
                   metis.getEind());
  graph.graphOutput(outBaseFilename);
}

TEST(ParserVTest, adder) {
  parse("adder");
}

TEST(ParserVTest, c17) {
  parse("c17");
}

TEST(ParserVTest, arbiter) {
  parse("arbiter");
}

TEST(ParserVTest, bar) {
  parse("bar");
}

TEST(ParserVTest, c1355) {
  parse("c1355");
}

TEST(ParserVTest, c1908) {
  parse("c1908");
}

TEST(ParserVTest, c3540) {
  parse("c3540");
}

TEST(ParserVTest, c432) {
  parse("c432");
}

TEST(ParserVTest, c499) {
  parse("c499");
}

TEST(ParserVTest, c6288) {
  parse("c6288");
}

TEST(ParserVTest, c880) {
  parse("c880");
}

TEST(ParserVTest, cavlc) {
  parse("cavlc");
}

TEST(ParserVTest, ctrl) {
  parse("ctrl");
}

TEST(ParserVTest, dec) {
  parse("dec");
}

TEST(ParserVTest, div) {
  parse("div");
}

TEST(ParserVTest, i2c) {
  parse("i2c");
}

TEST(ParserVTest, int2float) {
  parse("int2float");
}

TEST(ParserVTest, log2) {
  parse("log2");
}

TEST(ParserVTest, max) {
  parse("max");
}

TEST(ParserVTest, multiplier) {
  parse("multiplier");
}

TEST(ParserVTest, router) {
  parse("router");
}

TEST(ParserVTest, sin) {
  parse("sin");
}

TEST(ParserVTest, sqrt) {
  parse("sqrt");
}

TEST(ParserVTest, square) {
  parse("square");
}

TEST(ParserVTest, voter) {
  parse("voter");
}
