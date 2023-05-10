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

using GateBindings = BoundGNet::GateBindings;

bool areEquivalent(BoundGNet bgnet1, BoundGNet bgnet2) {
  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap1, varMap2;

  for (size_t i = 0; i < bgnet1.inputBindings.size(); i++) {
    varMap1[bgnet1.inputBindings[i]] = x[i];
  }
  for (size_t i = 0; i < bgnet2.inputBindings.size(); i++) {
    varMap2[bgnet2.inputBindings[i]] = x[i];
  }

  BDD bdd1 = GNetBDDConverter::convert(*bgnet1.net,
                                       bgnet1.outputBindings[0],
                                       varMap1, manager);
  BDD bdd2 = GNetBDDConverter::convert(*bgnet2.net,
                                       bgnet2.outputBindings[0],
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
           (rwdb.get(truthTable)[0].inputBindings == GateBindings({1, 2, 3}))
           && (rwdb.get(truthTable)[0].outputBindings == GateBindings
           ({4, 5}));

  result = result && !rwdb.empty();
  rwdb.erase(truthTable);
  result = result && rwdb.empty();

  return result;
}

bool insertGetSQLiteRWDBTest() {
  SQLiteRWDatabase db;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    db.linkDB(dbPath);
    db.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = makeAnd(2, inputs, outputId1);
    GateBindings inputs1 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs1 = {outputId1};

    Gate::Id outputId2;
    inputs.clear();
    std::shared_ptr<GNet> dummy2 = makeOr(2, inputs, outputId2);
    GateBindings inputs2 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    BoundGNet::BoundGNetList bgl = {{dummy1, inputs1, outputs1},
                                    {dummy2, inputs2, outputs2}};

    db.insertIntoDB(truthTable, bgl);

    auto newBgl = db.get(truthTable);

    result = areEquivalent(bgl[0], newBgl[0]) &&
             areEquivalent(bgl[1], newBgl[1]);

    db.closeDB();
  } catch (const char* msg) {
    result = false;
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool updateSQLiteRWDBTest() {
  SQLiteRWDatabase db;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    db.linkDB(dbPath);
    db.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = std::make_shared<GNet>
                                   (*makeAnd(2, inputs, outputId1));
    GateBindings inputs1 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs1 = {outputId1};

    inputs.clear();
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = std::make_shared<GNet>
                                   (*makeOr(2, inputs, outputId2));
    GateBindings inputs2 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    BoundGNet::BoundGNetList bgl = {{dummy1, inputs1, outputs1}};
    BoundGNet::BoundGNetList newBgl = {{dummy2, inputs2, outputs2}};

    db.insertIntoDB(truthTable, bgl);

    db.updateInDB(truthTable, newBgl);

    auto gottenBgl = db.get(truthTable);

    result = areEquivalent(gottenBgl[0], newBgl[0]);

    db.closeDB();
  } catch (const char* msg) {
    result = false;
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool deleteSQLiteRWDBTest() {
  SQLiteRWDatabase db;
  std::string dbPath = "rwtest.db";
  bool result;

  try {
    db.linkDB(dbPath);
    db.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = std::make_shared<GNet>
                                   (*makeAnd(2, inputs, outputId1));
    GateBindings inputs1 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs1 = {outputId1};

    inputs.clear();
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = std::make_shared<GNet>
                                   (*makeOr(2, inputs, outputId2));
    GateBindings inputs2 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs2 = {outputId2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    BoundGNet::BoundGNetList bgl = {{dummy1, inputs1, outputs1},
                                    {dummy2, inputs2, outputs2}};

    db.insertIntoDB(truthTable, bgl);
    db.deleteFromDB(truthTable);

    result = !db.contains(truthTable);

    db.closeDB();
  } catch (const char* msg) {
    result = false;
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool pushSQLiteRWDBTest() {
  SQLiteRWDatabase db;
  std::string dbPath = "rwtest.db";
  bool result = true;

  try {
    db.linkDB(dbPath);
    db.openDB();

    TruthTable truthTable = TruthTable(1);

    Gate::SignalList inputs;
    Gate::Id outputId1;
    std::shared_ptr<GNet> dummy1 = std::make_shared<GNet>
                                   (*makeAnd(2, inputs, outputId1));
    GateBindings inputs1 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs1 = {outputId1};
    BoundGNet bnet1 = {dummy1, inputs1, outputs1};

    inputs.clear();
    Gate::Id outputId2;
    std::shared_ptr<GNet> dummy2 = std::make_shared<GNet>
                                   (*makeOr(2, inputs, outputId2));
    GateBindings inputs2 = {inputs[0].node(), inputs[1].node()};
    GateBindings outputs2 = {outputId2};
    BoundGNet bnet2 = {dummy2, inputs2, outputs2};

    dummy1->sortTopologically();
    dummy2->sortTopologically();

    db.pushInDB(truthTable, bnet1);
    result = result && db.dbContains(truthTable);
    db.pushInDB(truthTable, bnet2);

    BoundGNet::BoundGNetList bgl = db.getFromDB(truthTable);
    result = result && areEquivalent(bnet1, bgl[0]) && areEquivalent(bnet2, bgl[1]);
    db.closeDB();
  } catch (const char* msg) {
    result = false;
    std::cout << msg << std::endl;
  }
  remove(dbPath.c_str());
  return result;
}

bool serializeTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  BoundGNet bGNet;

  bGNet.net = std::make_shared<GNet>
                (*makeAnd(2, inputs, outputId));
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};
  bGNet.inputDelays = {exp(-100), exp(100)};

  std::string ser = SQLiteRWDatabase::serialize({bGNet});
  BoundGNet newBGNet = SQLiteRWDatabase::deserialize(ser)[0];

  return areEquivalent(bGNet, newBGNet) &&
         bGNet.inputDelays == newBGNet.inputDelays;
}

TEST(RWDatabaseTest, BasicTest) {
  EXPECT_TRUE(basicTest());
}

TEST(RWDatabaseTest, SerializeTest) {
  EXPECT_TRUE(serializeTest());
}

TEST(RWDatabaseTest, InsertGetSQLiteRWDBTest) {
  EXPECT_TRUE(insertGetSQLiteRWDBTest());
}

TEST(RWDatabaseTest, UpdateSQLiteRWDBTest) {
  EXPECT_TRUE(updateSQLiteRWDBTest());
}

TEST(RWDatabaseTest, DeleteSQLiteRWDBTest) {
  EXPECT_TRUE(deleteSQLiteRWDBTest());
}

TEST(RWDatabaseTest, PushSQLiteRWDBTest) {
  EXPECT_TRUE(pushSQLiteRWDBTest());
}
