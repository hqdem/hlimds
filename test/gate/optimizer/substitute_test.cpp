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

  void substitutePrint(const std::filesystem::path &subCatalog,
                       GNet *net, GNet *subNet, GateID cutFor,
                       const Cut &cut) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / "build" / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filename1 = outputPath / "gnet1.dot";
    std::string filename2 = outputPath / "gnet2.dot";
    std::string filename12 = outputPath / "gnet12.dot";

    Dot mainGnetDot(net);
    Dot subGnetDot(subNet);

    mainGnetDot.print(filename1);
    subGnetDot.print(filename2);

    substitute(cutFor, createPrimitiveMap(subNet, cut), subNet, net);

    mainGnetDot.print(filename12);
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
    auto g = gnet1Exteded(mainGnet);
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
    int optimization = fakeSubstitute(g[6], map, &subGnet,&mainGnet);

    EXPECT_EQ(0, optimization);

    substitutePrint("test/data/gate/optimizer/output/substituteCount", &mainGnet,
                    &subGnet, g[6], cut);

    EXPECT_EQ(7, mainGnet.nGates());
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
    int optimization = fakeSubstitute(g[12], map, &subGnet,&mainGnet);
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
    int optimization = fakeSubstitute(g.back(), map, &subGnet,
                                      &mainGnet);
     EXPECT_EQ(3, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceBigCut", &mainGnet,
                    &subGnet, g.back(), mainGnet.getSources());
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
    int optimization = fakeSubstitute(g.back(), map, &subGnet,
                                      &mainGnet);
    EXPECT_EQ(1, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceBigCut2", &mainGnet,
                    &subGnet, g.back(), cut);
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
    int optimization = fakeSubstitute(g.back(), map, &subGnet,
                                      &mainGnet);
    EXPECT_EQ(-4, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceSmallCut", &mainGnet,
                    &subGnet, g.back(), mainGnet.getSources());
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
    int optimization = fakeSubstitute(g.back(), map, &subGnet,
                                      &mainGnet);
    EXPECT_EQ(-3, optimization);

    substitutePrint("test/data/gate/optimizer/output/DisbalanceSmallCut2", &mainGnet,
                    &subGnet, g.back(), mainGnet.getSources());
  }

} // namespace eda::gate::optimizer
