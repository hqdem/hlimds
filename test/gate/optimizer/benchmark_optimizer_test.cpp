//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"
#include "gate/printer/dot.h"
#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/strategy/exhaustive_search_optimizer.h"
#include "gtest/gtest.h"
#include "gate/optimizer/examples.h"

namespace eda::gate::optimizer {

  using lorina::text_diagnostics;
  using lorina::diagnostic_engine;
  using lorina::return_code;

  GNet *getNet(const std::string& infile) {
    const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path prefixPath = homePath / subCatalog;

    std::string filename = prefixPath / (infile + ".v");

    text_diagnostics consumer;
    diagnostic_engine diag(&consumer);

    GateVerilogParser parser(infile);

    return_code result = read_verilog(filename, parser, &diag);
    EXPECT_EQ(result, return_code::success);

    return parser.getGnet();
  }

  std::string getPath(const std::string& nameDir) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / "test/data/gate/optimizer/output" / nameDir;
    system(std::string("mkdir -p ").append(outputPath).c_str());
    return outputPath;
  }

  TEST(RewriteTest, gnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1(net);

    optimizePrint(&net, 4, getPath("gnet1_rewrite"), ExhausitiveSearchOptimizer());
  }

  TEST(RewriteTest, gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet2(net);

    optimizePrint(&net, 4, getPath("gnet2_rewrite"), ExhausitiveSearchOptimizer());
  }

  TEST(RewriteTest, gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet3(net);

    optimizePrint(&net, 4, getPath("gnet3_rewrite"), ExhausitiveSearchOptimizer());
  }

  TEST(RewriteTest, c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    auto net = getNet("c17");

    optimizePrint(net, 4, getPath("c17_rewrite"), ExhausitiveSearchOptimizer());

    delete net;
  }

  TEST(RewriteTest, c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    auto net = getNet("c432");

    optimizePrint(net, 6, getPath("c432_rewrite"), ExhausitiveSearchOptimizer());

    delete net;
  }

}