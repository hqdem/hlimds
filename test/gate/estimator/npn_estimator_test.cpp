//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/npn_estimator.h"
#include "gate/model/subnet.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

namespace eda::gate::estimator {

TEST(NpnEstimator, CheckExtendTTs) {
  const uint16_t k = 4;

  auto builder = translator::translateGmlOpenabc("ss_pcm_orig");

  NpnSettings settings(k, true, false);
  NpnStats stats;
  NpnEstimator estimator;
  estimator.estimate(builder, settings, stats);

  for (const auto &pair : stats) {
    EXPECT_EQ(pair.first.num_vars(), k);
  }
}

TEST(NpnEstimator, CheckNotCountTrivial) {
  const uint16_t nIn = 2;

  auto builder = std::make_shared<model::SubnetBuilder>();
  auto links = builder->addInputs(nIn);
  links.emplace_back(builder->addCell(model::CellSymbol::AND, links));

  builder->addOutput(links.back());

  NpnSettings settings(nIn, true, false);
  NpnStats stats;
  NpnEstimator estimator;
  estimator.estimate(builder, settings, stats);

  EXPECT_EQ(stats.size(), 1);
}

} // namespace eda::gate::estimator
