//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/transformer/bdd.h"
#include "gate/optimizer/ttbuilder.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {
  using BoundGNet = RWDatabase::BoundGNet;
  using GateList = std::vector<Gate::Id>;

  const uint64_t AND2_TRUTH_TABLE = 18446462598732840960ull;
  const uint64_t NOR2_TRUTH_TABLE = 65535ull;
  const uint64_t AND6_TRUTH_TABLE = 9223372036854775808ull;

  bool twoVarsBuildTest() {
    Gate::SignalList inputs;
    Gate::Id outputId;
    BoundGNet bgnet;

    bgnet.net = makeAnd(2, inputs, outputId);
    for (size_t i = 0; i < inputs.size(); i++) {
      bgnet.bindings[i] = inputs[i].node();
    }
    RWDatabase::TruthTable andTT = TTBuilder::build(bgnet);

    inputs.clear();
    bgnet.net = makeNor(2, inputs, outputId);
    for (size_t i = 0; i < inputs.size(); i++) {
      bgnet.bindings[i] = inputs[i].node();
    }
    RWDatabase::TruthTable norTT = TTBuilder::build(bgnet);

    return andTT == AND2_TRUTH_TABLE && norTT == NOR2_TRUTH_TABLE;
  }

  bool and6BuildTest() {
    Gate::SignalList inputs;
    Gate::Id outputId;
    BoundGNet bgnet;

    bgnet.net = makeAnd(6, inputs, outputId);
    for (size_t i = 0; i < inputs.size(); i++) {
      bgnet.bindings[i] = inputs[i].node();
    }
    RWDatabase::TruthTable andTT = TTBuilder::build(bgnet);

    return andTT == AND6_TRUTH_TABLE;
  }

  TEST(TTBuilderTest, TwoVarsBuildTest) {
    EXPECT_TRUE(twoVarsBuildTest());
  }

  TEST(TTBuilderTest, And6BuildTest) {
    EXPECT_TRUE(and6BuildTest());
  }
}
