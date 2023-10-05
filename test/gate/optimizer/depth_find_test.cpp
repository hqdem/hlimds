//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/model/utils.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/printer/dot.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

  void testDepthFinder(const std::function<void(GNet &)> &netCreator,
                       const std::string &graphFileName,
                       const int expectedDepth) {

    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    netCreator(net);

    Dot dot(&net);
    dot.print(createOutPath("depth_find/").c_str() + graphFileName);

    EXPECT_EQ(expectedDepth, getNetDepth(net));
  }

  TEST(DepthFindTest, gnet1) {
    testDepthFinder(gnet1, "gnet1.dot", 4);
  }

  TEST(DepthFindTest, gnet2) {
    testDepthFinder(gnet2, "gnet2.dot", 3);
  }

  TEST(DepthFindTest, gnet1Extended) {
    testDepthFinder(gnet1Extended, "gnet1Extended.dot", 5);
  }

  TEST(DepthFindTest, gnet2Extended) {
    testDepthFinder(gnet2Extended, "gnet2Extended.dot", 3);
  }

  TEST(DepthFindTest, gnet3) {
    testDepthFinder(gnet3, "gnet3.dot", 5);
  }

  TEST(DepthFindTest, gnet3Cone) {
    testDepthFinder(gnet3Cone, "gnet3Cone.dot", 3);
  }

  TEST(DepthFindTest, gnet4) {
    testDepthFinder(gnet4, "gnet4.dot", 2);
  }

} // namespace eda::gate::optimizer
