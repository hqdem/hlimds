//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/optimizer/strategy/apply_search_optimizer.h"
#include "gate/optimizer/strategy/exhaustive_search_optimizer.h"
#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"

#include "gtest/gtest.h"

using namespace eda::gate::parser;

namespace eda::gate::optimizer {

  using lorina::text_diagnostics;
  using lorina::diagnostic_engine;
  using lorina::return_code;

  const int kCutsMax = 100;

  TEST(RewriteTest, gnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1(net);

    optimizePrint(&net, 4, createOutPath("gnet1_rewrite"),
                  ExhausitiveSearchOptimizer(), kCutsMax);
  }

  TEST(RewriteTest, gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet2(net);

    optimizePrint(&net, 4, createOutPath("gnet2_rewrite"),
                  ExhausitiveSearchOptimizer(), kCutsMax);
  }

  TEST(RewriteTest, gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet3(net);

    optimizePrint(&net, 4, createOutPath("gnet3_rewrite"),
                  ExhausitiveSearchOptimizer(), kCutsMax);
  }

  TEST(RewriteTest, c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    auto net = parseVerilog("c17.v");

    std::cout << "Gates number before rewrite : " << net->nGates() << std::endl;

    optimize(net, 6, ExhausitiveSearchOptimizer(), kCutsMax);

    std::cout << "Gates number before rewrite : " << net->nGates() << std::endl;

    delete net;
  }

  TEST(RewriteTest, c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    auto net = parseVerilog("c432.v");

    std::cout << "Gates number before rewrite : " << net->nGates() << std::endl;

    optimize(net, 4, ApplySearchOptimizer(), kCutsMax);

    std::cout << "Gates number before rewrite : " << net->nGates() << std::endl;

    delete net;
  }

  // TODO: uncomment the test below when incident #21 will be fixed
  /*TEST(RewriteTest, adder) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    auto net = parseVerilog("adder.v");

    std::cout << "Gates number before rewrite : " << net->nGates() << std::endl;

    optimize(net, 4, ApplySearchOptimizer());

    std::cout << "Gates number after rewrite : " << net->nGates() << std::endl;

    delete net;
  }*/

}
