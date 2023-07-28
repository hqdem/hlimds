//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/examples.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/optimizer_util.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

  void substitutePrint(const std::filesystem::path &subCatalog,
                       GNet *net, GNet *substNet, GateID cutFor,
                       const Cut &cut) {

    std::filesystem::path outputPath = createOutPath(subCatalog);

    std::string toSubstitute = outputPath / "gnet1.dot";
    std::string substituteWith = outputPath / "gnet2.dot";
    std::string afterSubstitute = outputPath / "gnet12.dot";

    Dot mainGnetDot(net);
    Dot subGnetDot(substNet);

    mainGnetDot.print(toSubstitute);
    subGnetDot.print(substituteWith);

    auto map = createPrimitiveMap(substNet, cut);

    NetSubstitute netSubstitute(cutFor, &map, substNet, net);

    netSubstitute.fakeSubstitute();
    netSubstitute.substitute();

    mainGnetDot.print(afterSubstitute);
  }

  TEST(SubstituteTest, substitute) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute", &mainGnet,
                    &subGnet, g[6], mainGnet.getSources());
    EXPECT_EQ(mainGnet.nGates(), subGnet.nGates());
  }

  TEST(SubstituteTest, substitute2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet mainGnet;
    GNet subGnet;
    auto g = gnet1Extended(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute2", &mainGnet,
                    &subGnet, g[6], mainGnet.getSources());

    EXPECT_EQ(mainGnet.nGates(), subGnet.nGates() + 2);
  }

//===----------------------------------------------------------------------===//

  TEST(SubstitueCount, gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet mainGnet;
    auto g = gnet2(mainGnet);

    GNet subGnet;
    gnet3Cone(subGnet);

    auto cut = {g[0], g[1], g[5]};
    auto map = createPrimitiveMap(&subGnet, cut);

    NetSubstitute netSubstitute(g[6], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(0, optimization);

    substitutePrint("test/data/gate/optimizer/output/substituteCount", &mainGnet,
                    &subGnet, g[6], cut);

    EXPECT_EQ(8, mainGnet.nGates());
  }

  TEST(SubstitueCount, gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet subGnet;
    GNet mainGnet;

    auto g = gnet3(mainGnet);
    gnet4(subGnet);

    auto cut = {g[0], g[3], g[7]};
    auto map = createPrimitiveMap(&subGnet, cut);

    NetSubstitute netSubstitute(g[12], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();
    EXPECT_EQ(0, optimization);

    substitutePrint("test/data/gate/optimizer/output/substituteCount2", &mainGnet,
                    &subGnet, g[12], cut);

    // Checking that link between 0 and 7 is saved.
    EXPECT_EQ(3, Gate::get(g[0])->links().size());
  }

  TEST(SubstitueCount, DisbalanceBigCut) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet subGnet;
    GNet mainGnet;

    auto g = gnet4(mainGnet);
    gnet2(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());

    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();
    EXPECT_EQ(3, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceBigCut", &mainGnet,
                    &subGnet, g[g.size() - 2], mainGnet.getSources());
  }

  TEST(SubstitueCount, DisbalanceBigCut2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet subGnet;
    GNet mainGnet;

    auto g = gnet2(mainGnet);
    gnet4(subGnet);

    auto cut = {g[4], g[5]};
    auto map = createPrimitiveMap(&subGnet, cut);

    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(1, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceBigCut2", &mainGnet,
                    &subGnet, g[g.size() - 2], cut);
  }

  TEST(SubstitueCount, DisbalanceSmallCut) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet subGnet;
    GNet mainGnet;

    auto g = gnet3(mainGnet);
    gnet4(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[15], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(-4, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceSmallCut", &mainGnet,
                    &subGnet, g[15], mainGnet.getSources());
  }

  TEST(SubstitueCount, DisbalanceSmallCut2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet subGnet;
    GNet mainGnet;

    auto g = gnet2(mainGnet);
    gnet4(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,&mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(-3, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceSmallCut2", &mainGnet,
                    &subGnet, g[6], mainGnet.getSources());
  }

} // namespace eda::gate::optimizer
