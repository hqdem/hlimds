//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/truthtable.h"

#include "gtest/gtest.h"

using namespace eda::gate::optimizer;
using namespace eda::gate::model;

using Gate = eda::gate::model::Gate;
using GateList = std::vector<Gate::Id>;
using GateSymbol = eda::gate::model::GateSymbol;
using GNet = eda::gate::model::GNet;

const TruthTable AND2_TRUTH_TABLE = TruthTable(9838263505978427528ull);
const TruthTable NOR2_TRUTH_TABLE = TruthTable(1229782938247303441ull);
const TruthTable AND6_TRUTH_TABLE = TruthTable(9223372036854775808ull);

bool twoVarsBuildTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  BoundGNet bGNet;
  bGNet.net = std::make_shared<GNet>(*makeAnd(2, inputs, outputId));
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};

  TruthTable andTT = TruthTable::build(bGNet);

  inputs.clear();
  bGNet.net = std::make_shared<GNet>(*makeNor(2, inputs, outputId));
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};
  TruthTable norTT = TruthTable::build(bGNet);

  return andTT == AND2_TRUTH_TABLE && norTT == NOR2_TRUTH_TABLE;
}

bool and6BuildTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  BoundGNet bGNet;

  bGNet.net = std::make_shared<GNet>(*makeAnd(6, inputs, outputId));
  bGNet.inputBindings.resize(6);
  for (int i = 0; i < 6; i++) {
    bGNet.inputBindings[i] = inputs[i].node();
  }
  bGNet.outputBindings = {outputId};

  TruthTable andTT = TruthTable::build(bGNet);

  return andTT == AND6_TRUTH_TABLE;
}

TEST(TruthTableTest, TwoVarsBuildTest) {
  EXPECT_TRUE(twoVarsBuildTest());
}

TEST(TruthTableTest, And6BuildTest) {
  EXPECT_TRUE(and6BuildTest());
}
