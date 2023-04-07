//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/transformer/bdd.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::gate::transformer;

using BDDList = GNetBDDConverter::BDDList;
using GateBDDMap = GNetBDDConverter::GateBDDMap;
using GateList = std::vector<Gate::Id>;
using GateUintMap = GNetBDDConverter::GateUintMap;

bool areEquivalent(BoundGNet bgnet1, BoundGNet bgnet2) {
  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap1, varMap2;

  for (size_t i = 0; i < bgnet1.inputs.size(); i++) {
    varMap1[bgnet1.inputs[i]] = x[i];
  }
  for (size_t i = 0; i < bgnet2.inputs.size(); i++) {
    varMap2[bgnet2.inputs[i]] = x[i];
  }

  BDD bdd1 = GNetBDDConverter::convert(*bgnet1.net,
                                       bgnet1.outputs[0],
                                       varMap1, manager);
  BDD bdd2 = GNetBDDConverter::convert(*bgnet2.net,
                                       bgnet2.outputs[0],
                                       varMap2, manager);

  return bdd1 == bdd2;
}

bool basicTest() {
  RWDatabase rwdb;
  bool result = true;

  std::shared_ptr<GNet> dummy = std::make_shared<GNet>();
  TruthTable truthTable = TruthTable(1);

  rwdb.set(truthTable, {{dummy, {1, 2, 3}, {4, 5}}});
  result = result && (rwdb.get(truthTable)[0].net == dummy) &&
           (rwdb.get(truthTable)[0].inputs == std::vector<Gate::Id>({1, 2, 3}))
           && (rwdb.get(truthTable)[0].outputs == std::vector<Gate::Id>
           ({4, 5}));

  result = result && !rwdb.empty();
  rwdb.erase(truthTable);
  result = result && rwdb.empty();

  return result;
}

bool insertGetARWDBTest() {
  SQLiteRWDatabase arwdb;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    arwdb.linkDB(dbPath);
    arwdb.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = makeAnd(2, inputs, outputId1);
    std::vector<Gate::Id> inputs1 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs1 = {outputId1};

    Gate::Id outputId2;
    inputs.clear();
    std::shared_ptr<GNet> dummy2 = makeOr(2, inputs, outputId2);
    std::vector<Gate::Id> inputs2 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, inputs1, outputs1},
                                     {dummy2, inputs2, outputs2}};

    arwdb.insertIntoDB(truthTable, bgl);

    auto newBgl = arwdb.get(truthTable);

    result = areEquivalent(bgl[0], newBgl[0]) &&
             areEquivalent(bgl[1], newBgl[1]);

    arwdb.closeDB();
  } catch (const char* msg) {
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool updateARWDBTest() {
  SQLiteRWDatabase arwdb;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    arwdb.linkDB(dbPath);
    arwdb.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = std::make_shared<GNet>
                                   (*makeAnd(2, inputs, outputId1));
    std::vector<Gate::Id> inputs1 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs1 = {outputId1};

    inputs.clear();
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = std::make_shared<GNet>
                                   (*makeOr(2, inputs, outputId2));
    std::vector<Gate::Id> inputs2 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, inputs1, outputs1}};
    RWDatabase::BoundGNetList newBgl = {{dummy2, inputs2, outputs2}};

    arwdb.insertIntoDB(truthTable, bgl);

    arwdb.updateInDB(truthTable, newBgl);

    auto gottenBgl = arwdb.get(truthTable);

    result = areEquivalent(gottenBgl[0], newBgl[0]);

    arwdb.closeDB();
  } catch (const char* msg) {
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool deleteARWDBTest() {
  SQLiteRWDatabase arwdb;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    arwdb.linkDB(dbPath);
    arwdb.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = std::make_shared<GNet>
                                   (*makeAnd(2, inputs, outputId1));
    std::vector<Gate::Id> inputs1 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs1 = {outputId1};

    inputs.clear();
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = std::make_shared<GNet>
                                   (*makeOr(2, inputs, outputId2));
    std::vector<Gate::Id> inputs2 = {inputs[0].node(), inputs[1].node()};
    std::vector<Gate::Id> outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, inputs1, outputs1},
                                     {dummy2, inputs2, outputs2}};

    arwdb.insertIntoDB(truthTable, bgl);
    arwdb.deleteFromDB(truthTable);

    result = !arwdb.contains(truthTable);

    arwdb.closeDB();
  } catch (const char* msg) {
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

TEST(RWDatabaseTest, BasicTest) {
  EXPECT_TRUE(basicTest());
}

TEST(RWDatabaseTest, InsertGetARWDBTest) {
  EXPECT_TRUE(insertGetARWDBTest());
}

TEST(RWDatabaseTest, UpdateARWDBTest) {
  EXPECT_TRUE(updateARWDBTest());
}

TEST(RWDatabaseTest, DeleteARWDBTest) {
  EXPECT_TRUE(deleteARWDBTest());
}
