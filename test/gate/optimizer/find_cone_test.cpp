//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/cone_visitor.h"
#include "gate/optimizer/optimizer_util.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

  GNet *findConePrint(const std::filesystem::path &subCatalog, GNet *net,
                      const std::vector<GateId> &cuNodes, GNet::V start) {
    std::filesystem::path outputPath = createOutPath(subCatalog);
    std::string wholeNet = outputPath / "cone0.dot";
    std::string extractedCone = outputPath / "cone.dot";

    Dot printer(net);
    printer.print(wholeNet);

    Cut cut;
    for (auto node: cuNodes) {
      cut.emplace(node);
    }

    ConeVisitor coneVisitor(cut, start);
    Walker walker(net, &coneVisitor);
    walker.walk(cut, start, false);

    GNet *subnet = coneVisitor.getGNet();

    printer = Dot(subnet);
    printer.print(extractedCone);

    return subnet;
  }

  TEST(FindConeTest, findCone) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto *cone = findConePrint("findCone1",
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

    auto cone = findConePrint("findCone2", &net,
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

    auto cone = findConePrint("findCone3_0",
                              &net, {g[0], g[3], g[7]}, g[8]);
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

    auto cone = findConePrint("findCone3_1",
                              &net, {g[0], g[3], g[7]}, g[12]);
    EXPECT_EQ(6, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(FindConeTest, findConeExcessiveCut) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto cone = findConePrint(
        "findConeExcessiveCut", &net,
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

    auto cone = findConePrint("findConeTrivial",
                              &net, {g[5]}, g[5]);
    EXPECT_EQ(2, cone->nGates());
    delete cone;
  }

  TEST(FindConeTest, findConeConstInputs) {
    GNet net;
    auto g = constInputs(net);
    auto cone = findConePrint("findConeConstInputs",
                              &net, {g[0], g[1], g[4]}, g[5]);

    bool hasZero = false;
    bool hasOne = false;
    bool hasIn = false;

    const auto &sources = cone->getSources();
    for (auto it = sources.begin(); it != sources.end(); ++it) {
      Gate *gate = Gate::get(*it);
      hasIn |= gate->isSource();
      hasZero |= gate->func() == model::GateSymbol::ZERO;
      hasOne |= gate->func() == model::GateSymbol::ONE;
    }

    ASSERT_TRUE(hasZero);
    ASSERT_TRUE(hasOne);
    ASSERT_TRUE(hasIn);

    delete cone;
  }

  TEST(FindConeTest, findConeFunction) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1ChangedFunc(net);

    std::vector<GateId> cut = {g[2], g[4]};
    BoundGNet binding = extractCone(&net, g[5], cut);

    GNet *cone = binding.net.get();
    const auto &matchMap = binding.inputBindings;

    EXPECT_EQ(4, cone->nGates());

    EXPECT_EQ(2, matchMap.size());
    EXPECT_EQ(Gate::get(cut[0])->links().size(),
              Gate::get(matchMap[0])->links().size());
    EXPECT_EQ(Gate::get(cut[1])->links().size(),
              Gate::get(matchMap[1])->links().size());
  }

} // namespace eda::gate::optimizer