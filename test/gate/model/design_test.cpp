//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/design.h"
#include "gate/model/examples.h"
#include "gate/model/net.h"
#include "gate/optimizer/pass.h"
#include "test_util.h"

#include "gtest/gtest.h"

#include <cstddef>

namespace eda::gate::model {

using namespace eda::gate::debugger;
using namespace eda::gate::optimizer;

using NetBuilder = model::NetBuilder;

static const std::string designOutPath =
    "test/gate/model/design/";

static void test(const DesignBuilderPtr &builder) {
  EXPECT_TRUE(builder->getSubnetNum() != 0);

  foreach(aig())->transform(builder);
  builder->save("premapped");

  foreach(resyn())->transform(builder);
  builder->save("optimized");

  const auto &checker = SatChecker::get();
  const auto result = checker.areEquivalent(*builder, "premapped", "optimized");
  EXPECT_TRUE(result.equal());

  const auto premappedNetID = builder->make("premapped");
  EXPECT_TRUE(premappedNetID != OBJ_NULL_ID);

  const auto optimizedNetID = builder->make("optimized");
  EXPECT_TRUE(optimizedNetID != OBJ_NULL_ID);

#ifdef UTOPIA_DEBUG
  std::cout << Net::get(premappedNetID) << std::endl;
  std::cout << Net::get(optimizedNetID) << std::endl;
#endif // UTOPIA_DEUB
}

void printDesign(const DesignBuilderPtr &builder, const std::string &name) {
  std::ofstream out;
  std::filesystem::path filePath = createOutDir(designOutPath);
  out.open(filePath.c_str() + name + ".dot");
  out << *builder << '\n';
  out.close();
}

TEST(DesignTest, RandomSubnet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto subnetID = makeSubnetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(subnetID);
  test(builder);
  printDesign(builder, "random_subnet");
}

TEST(DesignTest, RandomNet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  test(builder);
  printDesign(builder, "random_net");
}

TEST(DesignTest, Print100) {
  const size_t nIn = 50;
  const size_t nOut = 50;
  const size_t nCell = 100;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeTriggerNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  printDesign(builder, "100_elements");
}

TEST(DesignTest, Print5000) {
  const size_t nIn = 1000;
  const size_t nOut = 1000;
  const size_t nCell = 5000;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeTriggerNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  printDesign(builder, "5000_elements");
}

} // namespace eda::gate::model
