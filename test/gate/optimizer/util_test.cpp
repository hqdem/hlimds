//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/optimizer/util.h"

#include "gtest/gtest.h"

using namespace eda::gate::parser;

namespace eda::gate::optimizer {

  void removeNodePrint(const std::filesystem::path &subCatalog, GNet *net,
                       GateId start) {
    std::filesystem::path outputPath = createOutPath(subCatalog);
    std::string beforeRm = outputPath / "beforeDelete.dot";
    std::string afterRm = outputPath / "afterDelete.dot";

    Dot printer(net);
    printer.print(beforeRm);

    rmRecursive(net, start);

    printer = Dot(net);
    printer.print(afterRm);
  }

  TEST(RmNodeTest, WithoutParent) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    std::filesystem::path subPath = "RmNodeTest";
    removeNodePrint(subPath / "WithoutParent", &net, g[2]);

    // Check that correct number of nodes are remained.
    EXPECT_EQ(7, net.nGates());
  }

  TEST(RmNodeTest, LastNode) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    std::filesystem::path subPath = "RmNodeTest";
    removeNodePrint(subPath / "LastNode", &net, g.back());

    // Check that all nodes are removed.
    EXPECT_EQ(0, net.nGates());
  }

  TEST(RmNodeTest, MidNodes) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    std::filesystem::path subPath = "RmNodeTest";
    removeNodePrint(subPath / "MidNodes", &net, g[13]);

    // Check that correct number of nodes are remained.
    EXPECT_EQ(15, net.nGates());
  }

} // namespace eda::gate::optimizer
