//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/optimizer/pass.h"
#include "gate/optimizer/subnet_transformer.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

#include <chrono>
#include <unordered_map>
#include <vector>

namespace eda::gate::optimizer {

void runDbSynthesizer(const std::string &info,
                      const SubnetPass &p,
                      SubnetBuilderPtr &builder,
                      model::SubnetID beforeID) {

  const auto start{std::chrono::steady_clock::now()};
  p->transform(builder);
  const auto end{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> duration{end - start};

  std::cout << "Time of " << info << " " << duration.count() << " seconds\n";
  const auto afterID = builder->make(true);
  const auto &subnet = model::Subnet::get(afterID);
  std::cout << "Size after "  << info << " " << subnet.size() << std::endl;
  std::cout << "Depth after " << info << " " << subnet.getPathLength().second
                                             << std::endl;

  debugger::SatChecker &checker = debugger::SatChecker::get();
  EXPECT_TRUE(model::Subnet::get(beforeID).size() >= subnet.size());
  EXPECT_TRUE(checker.areEquivalent(beforeID, afterID).equal());
}

void runDbSynthesizer(const std::string &file) {
  const auto subnetID = translator::translateGmlOpenabc(file)->make();
  const auto &subnet = model::Subnet::get(subnetID);

  const auto standartRw = eda::gate::optimizer::rw();
  const auto generationRw = eda::gate::optimizer::rwxag4();

  auto builder1 = std::make_shared<model::SubnetBuilder>(subnetID);
  auto builder2 = std::make_shared<model::SubnetBuilder>(subnetID);

  std::cout << "Size before: " << subnet.size() << std::endl;
  std::cout << "Depth before: " << subnet.getPathLength().second << std::endl;

  runDbSynthesizer("ABC RW:", standartRw, builder1, subnetID);
  runDbSynthesizer("NPN4 RW:", generationRw, builder2, subnetID);
}

TEST(DbSynthesizer, i2c) {
  runDbSynthesizer("i2c_orig");
}

TEST(DbSynthesizer, sasc) {
  runDbSynthesizer("sasc_orig");
}

TEST(DbSynthesizer, ss_pcm) {
  runDbSynthesizer("ss_pcm_orig");
}

} // namespace eda::gate::optimizer
