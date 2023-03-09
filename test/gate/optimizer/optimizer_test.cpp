//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/check_cut.h"
#include "gate/optimizer/optimizer.h"
#include "gate/parser/gate_verilog_parser.h"
#include "gtest/gtest.h"

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <string>

using Vertex = eda::gate::model::GNet::V;

TEST(OptimizerTest, c17) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPath = homePath / subCatalog;
  const std::string infile = "c17";

  std::string filename = prefixPath / (infile + ".v");
  lorina::text_diagnostics consumer;
  lorina::diagnostic_engine diag(&consumer);

  GateVerilogParser parser(infile);
  assert(lorina::return_code::success == read_verilog(filename, parser, &diag));

  Optimizer optimizer(parser.getGnet());
  optimizer.optimize(4);

  for (auto &[v, cs]: optimizer.getCutStorage().cuts) {
    for (const auto &c: cs) {
      Vertex failed;

      if (!isCut(v, c, failed)) {
        FAIL() << "Wrong cut for v " << v << "; failed " << failed << "\n";
      }
    }
  }
}

