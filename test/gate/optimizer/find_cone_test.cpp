//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/examples.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

  GNet *findConePrint(const std::filesystem::path &subCatalog, GNet *net,
                      const std::vector<GateID> &cuNodes, GNet::V start) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / "build" / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filename1 = outputPath / "cone0.dot";
    std::string filename2 = outputPath / "cone.dot";

    Dot printer(net);
    printer.print(filename1);

    Cut cut;
    for (auto node: cuNodes) {
      cut.emplace(node);
    }

    ConeVisitor coneVisitor(cut);
    Walker walker(net, &coneVisitor, nullptr);
    walker.walk(start, cut,  false);

    GNet *subnet = coneVisitor.getGNet();

    printer = Dot(subnet);
    printer.print(filename2);

    return subnet;
  }

  TEST(FindConeTest, findCone) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto *cone = findConePrint("test/data/gate/optimizer/output/findCone1",
                               &net, {g[2], g[4]}, g[5]);
    EXPECT_EQ(4, cone->nGates());
    delete cone;
  }

  TEST(FindConeTest, findCone2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findCone2", &net,
                              {g[2], g[3], g[4], g[6], g[7]}, g[14]);
    EXPECT_EQ(8, cone->nGates());
    delete cone;
  }

  TEST(FindConeTest, findCone3_0) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findCone3_0",
                              &net,
                              {g[0], g[3], g[7]}, g[8]);
    EXPECT_EQ(5, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(FindConeTest, findCone3_1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findCone3_1",
                              &net,
                              {g[0], g[3], g[7]}, g[12]);
    EXPECT_EQ(6, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(FindConeTest, findConeExessiveCut) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto cone = findConePrint(
            "test/data/gate/optimizer/output/findConeExessiveCut", &net,
            {g[0], g[1], g[2], g[4]}, g[5]);
    EXPECT_EQ(4, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(FindConeTest, findConeTrivial) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findConeTrivial",
                              &net, {g[5]}, g[5]);
    EXPECT_EQ(2, cone->nGates());
    delete cone;
  }

} // namespace eda::gate::optimizer
