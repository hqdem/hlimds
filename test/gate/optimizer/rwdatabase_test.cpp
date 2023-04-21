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

#include <cmath>

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::gate::transformer;

using BDDList = GNetBDDConverter::BDDList;
using GateBDDMap = GNetBDDConverter::GateBDDMap;
using GateList = std::vector<Gate::Id>;
using GateUintMap = GNetBDDConverter::GateUintMap;


bool areEquivalent(RWDatabase::BoundGNet bgnet1, RWDatabase::BoundGNet bgnet2) {
  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap1, varMap2;

  for (auto p : bgnet1.bindings) {
    varMap1[p.second] = x[p.first];
  }
  for (auto p : bgnet2.bindings) {
    varMap2[p.second] = x[p.first];
  }

  BDD bdd1 = GNetBDDConverter::convert(*bgnet1.net,
                                       bgnet1.net->
                                       targetLinks().begin()->target,
                                       varMap1, manager);
  BDD bdd2 = GNetBDDConverter::convert(*bgnet2.net,
                                       bgnet2.net->
                                       targetLinks().begin()->target,
                                       varMap2, manager);

  return bdd1 == bdd2;
}


bool basicTest() {
  RWDatabase rwdb;
  bool result = true;

  std::shared_ptr<GNet> dummy = std::make_shared<GNet>();
  RWDatabase::GateBindings bindings = {{0, 1}, {1, 3}};
  RWDatabase::TruthTable truthTable = 8;

  rwdb.set(truthTable, {{dummy, bindings}});
  result = result && ((rwdb.get(truthTable)[0].net == dummy) &&
           (rwdb.get(truthTable)[0].bindings == bindings));

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

    RWDatabase::TruthTable truthTable = 1;

    Gate::SignalList inputs1;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = makeAnd(2, inputs1, outputId1);
    RWDatabase::GateBindings bindings1 = {{0, inputs1[0].node()},
                                          {1, inputs1[1].node()}};

    Gate::SignalList inputs2;
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = makeOr(2, inputs2, outputId2);
    RWDatabase::GateBindings bindings2 = {{0, inputs2[0].node()},
                                          {1, inputs2[1].node()}};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, bindings1},
                                     {dummy2, bindings2}};

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

    RWDatabase::TruthTable truthTable = 1;

    Gate::SignalList inputs1;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = makeAnd(2, inputs1, outputId1);
    RWDatabase::GateBindings bindings1 = {{0, inputs1[0].node()},
                                          {1, inputs1[1].node()}};

    Gate::SignalList inputs2;
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = makeOr(2, inputs2, outputId2);
    RWDatabase::GateBindings bindings2 = {{0, inputs2[0].node()},
                                          {1, inputs2[1].node()}};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, bindings1}};
    RWDatabase::BoundGNetList newBgl = {{dummy2, bindings2}};

    arwdb.insertIntoDB(truthTable, bgl);

    arwdb.updateInDB(truthTable, {{dummy2, bindings2}});

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

    RWDatabase::TruthTable truthTable = 1;

    Gate::SignalList inputs1;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = makeAnd(2, inputs1, outputId1);
    RWDatabase::GateBindings bindings1 = {{0, inputs1[0].node()},
                                          {1, inputs1[1].node()}};

    Gate::SignalList inputs2;
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = makeOr(2, inputs2, outputId2);
    RWDatabase::GateBindings bindings2 = {{0, inputs2[0].node()},
                                          {1, inputs2[1].node()}};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    RWDatabase::BoundGNetList bgl = {{dummy1, bindings1}, {dummy2, bindings2}};

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

bool serializeTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  RWDatabase::BoundGNet bGNet;

  bGNet.net = makeAnd(2, inputs, outputId);
  bGNet.bindings = {{0, inputs[0].node()},
                    {1, inputs[1].node()}};
  bGNet.inputsDelay = {{0, exp(-100)},
                       {1, exp(100)}};

  std::string ser = SQLiteRWDatabase::serialize({bGNet});
  RWDatabase::BoundGNet newBGNet = SQLiteRWDatabase::deserialize(ser)[0];

  return areEquivalent(bGNet, newBGNet) &&
         bGNet.inputsDelay == newBGNet.inputsDelay;
}

TEST(RWDatabaseTest, BasicTest) {
  EXPECT_TRUE(basicTest());
}

TEST(RWDatabaseTest, SerializeTest) {
  EXPECT_TRUE(serializeTest());
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
