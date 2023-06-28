//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gtest/gtest.h"
#include "gate/printer/dot.h"
#include "gate/optimizer/strategy/zero_optimizer.h"
#include "gate/optimizer/examples.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

  void rewritePrint(const std::filesystem::path &subCatalog, GNet *net) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / "build" / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filenameBefore = outputPath / "gnet.dot";
    std::string filenameAfter = outputPath / "gnet_rewritten.dot";

    Dot dot(net);
    dot.print(filenameBefore);

    optimize(net, 4, ZeroOptimizer());

    dot.print(filenameAfter);
  }

  void rewriteTrackPrint(const std::filesystem::path &subCatalog, GNet *net) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    optimizePrint(net, 4, outputPath, ZeroOptimizer());
  }

  // TODO: uncomment tests below when incident #7 will be fixed

  /*TEST(OptimizerTest, optimizeToZeroNetGnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1(net);
    rewritePrint("test/data/gate/optimizer/output/rewrite1", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet1Extended) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1Exteded(net);
    rewritePrint("test/data/gate/optimizer/output/rewrite1ex", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet2(net);
    rewriteTrackPrint("test/data/gate/optimizer/output/rewrite2", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet3(net);
    optimizePrint(&net, 4, "test/data/gate/optimizer/output/rewrite3",
                  ZeroOptimizer());
  }*/

}
