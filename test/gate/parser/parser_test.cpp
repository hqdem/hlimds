//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/printer/dot.h"
#include "gtest/gtest.h"
#include "rtl/compiler/compiler.h"
#include "rtl/parser/ril/parser.h"

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <string>

using namespace eda::gate::model;
using namespace eda::gate::parser;
using namespace eda::gate::parser::verilog;
using namespace eda::rtl::compiler;
using namespace lorina;


namespace eda::gate::parser {

GNet *parseVerilogTest(const std::string &infile) {
  const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPath = homePath / subCatalog;

  std::string filename = prefixPath / infile;

  return eda::gate::parser::verilog::getNet(filename, infile);
}

std::unique_ptr<GNet> parseRil(const std::string &fileName,
                               const std::string &subPath) {
  std::filesystem::path basePath = std::getenv("UTOPIA_HOME");
  std::filesystem::path fullPath = basePath / subPath / fileName;

  auto model = eda::rtl::parser::ril::parse(fullPath);
  Compiler compiler(FLibraryDefault::get());
  return compiler.compile(*model);
}

GNet getModel(const std::string &fileName,
              const std::string &subPath) {

  Exts ext = getExt(fileName);
  switch (ext) {
  case Exts::VERILOG:
    return *parseVerilogTest(fileName);
  case Exts::RIL:
    return *parseRil(fileName, subPath);
  default:
    CHECK(false) << "Unsupported extension!" << std::endl;
    return GNet(0);
  }
}

Exts getExt(const std::string &fileName) {
  std::uint64_t pos = fileName.rfind(".");
  assert(pos != std::string::npos);

  std::string extension = fileName.substr(pos + 1);
  if (extension == "v") {
    return Exts::VERILOG;
  } else if (extension == "ril") {
    return Exts::RIL;
  }
  return Exts::UNSUPPORTED;
}

std::filesystem::path inputFile(const std::string &infile) {
  const std::filesystem::path subCatalog = "test/data/gate/parser";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPathIn =  homePath / subCatalog / "verilog";
  std::string filename = prefixPathIn / infile;
  return filename;
}

std::filesystem::path outFile(const std::string &infile) {
  const std::filesystem::path subCatalog = "test/data/gate/parser";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPathOut = homePath / "build" / subCatalog / "output";

  system(std::string("mkdir -p ").append(prefixPathOut).c_str());

  std::string outFilename = prefixPathOut / (infile + ".dot");
  return outFilename;
}

void parsePassed(const std::string &infile) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  std::string filename = inputFile(infile);
  std::string outFilename = outFile(infile);

  text_diagnostics consumer;
  diagnostic_engine diag(&consumer);
  GateVerilogParser parser(infile);

  return_code result = read_verilog(filename, parser, &diag);
  EXPECT_EQ(result, return_code::success);

  Dot dot(parser.getGnet());
  dot.print(outFilename);

  delete parser.getGnet();
}

void parseFailed(const std::string &infile) {
  std::string filename = inputFile(infile);

  text_diagnostics consumer;
  diagnostic_engine diag(&consumer);
  GateVerilogParser parser(infile);

  return_code result = read_verilog(filename, parser, &diag);
  EXPECT_FALSE(result == return_code::success);
}

size_t parseOuts(const std::string &infile) {
  return parseVerilogTest(infile)->nOuts();
}
} // namespace eda::gate::parser

TEST(LorinaTest, adder) {
  parsePassed("adder.v");
}

TEST(LorinaTest, c17) {
  parsePassed("c17.v");
}

TEST(LorinaTest, c17_modified) {
  parsePassed("c17_modified.v");
}

TEST(LorinaTest, arbiter) {
  parsePassed("arbiter.v");
}

TEST(LorinaTest, bar) {
  parsePassed("bar.v");
}

TEST(LorinaTest, c1355) {
  parsePassed("c1355.v");
}

TEST(LorinaTest, c1908) {
  parsePassed("c1908.v");
}

TEST(LorinaTest, c3540) {
  parsePassed("c3540.v");
}

TEST(LorinaTest, c432) {
  parsePassed("c432.v");
}

TEST(LorinaTest, c499) {
  parsePassed("c499.v");
}

TEST(LorinaTest, c6288) {
  parsePassed("c6288.v");
}

TEST(LorinaTest, c7552) {
  parseFailed("c7552.v");
}

TEST(LorinaTest, c880) {
  parsePassed("c880.v");
}

TEST(LorinaTest, cavlc) {
  parsePassed("cavlc.v");
}

TEST(LorinaTest, ctrl) {
  parsePassed("ctrl.v");
}

TEST(LorinaTest, dec) {
  parsePassed("dec.v");
}

TEST(LorinaTest, div) {
  parsePassed("div.v");
}

TEST(LorinaTest, i2c) {
  parsePassed("i2c.v");
}

TEST(LorinaTest, int2float) {
  parsePassed("int2float.v");
}

TEST(LorinaTest, log2) {
  parsePassed("log2.v");
}

TEST(LorinaTest, max) {
  parsePassed("max.v");
}

TEST(LorinaTest, multiplier) {
  parsePassed("multiplier.v");
}

TEST(LorinaTest, router) {
  parsePassed("router.v");
}

TEST(LorinaTest, sin) {
  parsePassed("sin.v");
}

TEST(LorinaTest, sqrt) {
  parsePassed("sqrt.v");
}

TEST(LorinaTest, square) {
  parsePassed("square.v");
}

TEST(LorinaTest, voter) {
  parsePassed("voter.v");
}

// The test suite checks if the nets, built according to gate-level Verilog
// descriptions, contain the correct number of outputs.
TEST(LorinaOutTest, sqrt) {
  EXPECT_EQ(parseOuts("sqrt.v"), 64);
}

TEST(LorinaOutTest, adder) {
  EXPECT_EQ(parseOuts("adder.v"), 129);
}

TEST(LorinaOutTest, c17) {
  EXPECT_EQ(parseOuts("c17.v"), 2);
}
