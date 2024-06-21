//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/serializer.h"
#include "gate/model/examples.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "util/serializer.h"

#include "gtest/gtest.h"

#include <functional>

using namespace eda::gate::model;
using namespace eda::util;

static bool areEquivalent(const Subnet &lhs,
                          const Subnet &rhs) {
  return evaluate(lhs) == evaluate(rhs);
}

static void basicTestSubnet(const SubnetID &id) {
  SubnetSerializer ser;
  std::stringstream ss;
  ser.serialize(ss, id);
  SubnetID newId = ser.deserialize(ss);
  ASSERT_TRUE(areEquivalent(Subnet::get(id), Subnet::get(newId)));
}

std::vector<SubnetID> makeTestSubnets() {
  return {
    makeSubnet3AndOrXor(),
    makeSubnet4AndOr(),
    makeSubnetAndOrXor(),
    makeSubnetXorNorAndAndOr(),
    makeSubnetXorOrXor()
  };
}

TEST(SubnetSerializerTest, BasicTestSubnet) {
  auto testSubnets = makeTestSubnets();

  for (const auto &subnetID : testSubnets) {
    basicTestSubnet(subnetID);
  }
}

TEST(SubnetSerializerTest, BasicTestSubnetList) {
  auto testSubnets = makeTestSubnets();

  std::stringstream ss;
  SubnetListSerializer ser;
  ser.serialize(ss, testSubnets);
  auto newList = ser.deserialize(ss);
  EXPECT_EQ(testSubnets.size(), newList.size());
  for (size_t i = 0; i < testSubnets.size(); i++) {
    EXPECT_TRUE(areEquivalent(Subnet::get(testSubnets[i]),
                              Subnet::get(newList[i])));
  }
}

TEST(SubnetSerializerTest, TTSerializerTest) {
  using TT = kitty::dynamic_truth_table;
  TT tt1(12), tt2;
  TTSerializer s;
  for (int i = 0; i < 10; i++) {
    kitty::create_random(tt1);
    std::stringstream ss;
    s.serialize(ss, tt1);
    tt2 = s.deserialize(ss);
    if (tt1 != tt2) {
      std::cout << "Serializer error for this tt: ";
      kitty::print_binary(tt1, std::cout);
      std::cout << "\nIt turned into: ";
      kitty::print_binary(tt2, std::cout);
      std::cout << std::endl;
      ASSERT_TRUE(false);
    }
  }
  std::stringstream ss;
  std::vector<TT> tts(10);
  for (size_t i = 0; i < tts.size(); i++) {
    TT tt(15);
    kitty::create_random(tt);
    tts[i] = tt;
    s.serialize(ss, tt);
  }
  for (size_t i = 0; i < tts.size(); i++) {
    auto newTT = s.deserialize(ss);
    ASSERT_EQ(tts[i], newTT);
  }
  VectorSerializer<TT, TTSerializer> vs;
  vs.serialize(ss, tts);
  auto newTTs = vs.deserialize(ss);
  ASSERT_EQ(tts, newTTs);
}
