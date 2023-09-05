//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/examples.h"
#include "gate/optimizer/net_substitute.h"
#include "gtest/gtest.h"

namespace eda::gate::optimizer {

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount1) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1(mainGnet);
    gnet1(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(0, optimization);
  }

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount1ex) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1Extended(mainGnet);
    gnet1Extended(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(0, optimization);
  }

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount2) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet2(mainGnet);
    gnet2(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(0, optimization);
  }

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount12) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1(mainGnet);
    gnet2(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(0, optimization);
  }

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount12ex) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1Extended(mainGnet);
    gnet2(subGnet);

    auto map = createPrimitiveMap(&subGnet, mainGnet.getSources());
    NetSubstitute netSubstitute(g[6], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(1, optimization);
  }

  TEST(SubstituteOptimizationCount, optimizerEquivalentCount12Map) {
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet2Extended(mainGnet);
    auto gSub = gnet1(subGnet);

    std::unordered_map<GateId, GateId> map = {{gSub[0], g[0]},
                                              {gSub[1], g[2]},
                                              {gSub[2], g[1]},
                                              {gSub[3], g[3]}};
    NetSubstitute netSubstitute(g[g.size() - 2], &map, &subGnet,
                                &mainGnet);
    int optimization = netSubstitute.fakeSubstitute();

    EXPECT_EQ(1, optimization);
  }

} // namespace eda::gate::optimizer
