//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/npnstatdb.h"

#include "gtest/gtest.h"

#include <filesystem>

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::util;

using TT = kitty::dynamic_truth_table;

static bool areEquivalent(const Subnet &lhs,
                          const Subnet &rhs) {
  return evaluate(lhs) == evaluate(rhs);
}

static void deleteFileIfExists(const std::string &filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

static std::vector<SubnetID> getTransformedSubnets(NpnDatabase &db,
                                                   const TT &tt) {
  auto it = db.get(tt);
  std::vector<SubnetID> result;
  for (; it; it.next()) {
    result.push_back(it.get());
  }
  return result;
}

static void getTransformedSubnetsAndInfo(NpnDatabase &db,
                                         const TT &tt,
                                         std::vector<SubnetID> &subnets,
                                         std::vector<SubnetInfo> &infos) {
  auto it = db.get(tt);
  for (; it; it.next()) {
    subnets.push_back(it.get());
    infos.push_back(it.getInfo());
  }
}

TEST(NpnDatabaseSerializationTest, SavingDBIntoFile) {
  std::string filename = "test.rwdb";

  NpnDatabase npndb;

  SubnetID id1 = makeSubnet3AndOrXor();
  SubnetID id2 = makeSubnet4AndOr();
  npndb.push(id1);
  npndb.push(id2);
  npndb.push(id2);

  npndb.exportTo(filename);

  NpnDatabase npndbCopy = NpnDatabase::importFrom(filename);

  auto v = getTransformedSubnets(npndb, evaluate(Subnet::get(id1)).at(0));
  ASSERT_TRUE(v.size() == 1);
  ASSERT_TRUE(areEquivalent(Subnet::get(id1), Subnet::get(v[0])));

  v = getTransformedSubnets(npndb, evaluate(Subnet::get(id2)).at(0));
  ASSERT_TRUE(v.size() == 2);
  ASSERT_TRUE(areEquivalent(Subnet::get(id2), Subnet::get(v[1])));

  deleteFileIfExists("test.rwdb");
}

TEST(NpnDatabaseSerialization, SavingNpnStatDBIntoFile) {
  std::string filename = "test.rwdb";

  NpnStatDatabase npndb;

  SubnetBasis basis = {
    SubnetBasis::BasisElement::AND,
    SubnetBasis::BasisElement::OR
  };
  SubnetID id1 = makeSubnet3AndOrXor();
  SubnetID id2 = makeSubnet4AndOr();
  SubnetInfo in1 = {2, 3, 4, 5, 2, basis};
  npndb.push(id1, in1);
  npndb.push(id2, SubnetInfo::makeEmpty());

  npndb.exportTo(filename);

  NpnStatDatabase npndbCopy = NpnStatDatabase::importFrom(filename);

  std::vector<SubnetID> v;
  std::vector<SubnetInfo> infos;
  getTransformedSubnetsAndInfo(npndb, evaluate(Subnet::get(id1)).at(0), v,
                               infos);
  ASSERT_TRUE(v.size() == 1);
  ASSERT_TRUE(areEquivalent(Subnet::get(id1), Subnet::get(v[0])));

  ASSERT_EQ((uint16_t)infos[0].basis, (uint16_t)in1.basis);
  ASSERT_EQ(infos[0].inNum, in1.inNum);

  v = getTransformedSubnets(npndb, evaluate(Subnet::get(id2)).at(0));
  ASSERT_TRUE(v.size() == 1);
  ASSERT_TRUE(areEquivalent(Subnet::get(id2), Subnet::get(v.at(0))));

  deleteFileIfExists("test.rwdb");
}
