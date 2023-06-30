//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"
#include "gate/printer/dot.h"
#include "gtest/gtest.h"

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
  const std::filesystem::path prefixPathIn = prefixPath / "verilog";
  const std::filesystem::path prefixPathOut = prefixPath / "output";

  std::string filename = prefixPathIn / (infile + ".v");
  std::string outFilename = prefixPathOut / (infile + ".dot");

  text_diagnostics consumer;
  diagnostic_engine diag(&consumer);

  GateVerilogParser parser(infile);

  return_code result = read_verilog(filename, parser, &diag);
  EXPECT_EQ(result, return_code::success);

  Dot dot(parser.getGnet());
  dot.print(outFilename);

  delete parser.getGnet();
}
