//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/cell.h"
#include "gate/model/celltype.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/reed_muller.h"
#include "gate/optimizer/synthesis/zhegalkin.h"
#include "util/arith.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer::synthesis {

using DinTruthTable = kitty::dynamic_truth_table;
using Link = model::Subnet::Link;
using LinkList = std::vector<Link>;
using Polynomial = std::vector<uint64_t>;
using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

std::string generateRandom(const uint64_t size);
void testSubnetToSubnet(const Subnet &s1, const Subnet &s2);

void optimalEquality(uint64_t len) {
  std::string s = generateRandom(len);
  DinTruthTable t(len);
  ZhegalkinSynthesizer r;
  kitty::create_from_binary_string(t, s);
  auto &s1 = r.synthesize(t).makeObject();
  ReedMuller x1(sumOfTerms), x2(numberOfTerms), x3(longestTerm);
  
  auto &s2 = x1.synthesize(t).makeObject();
  auto &s3 = x2.synthesize(t).makeObject();
  auto &s4 = x3.synthesize(t).makeObject();

  testSubnetToSubnet(s1, s2);
  testSubnetToSubnet(s1, s3);
  testSubnetToSubnet(s1, s4);
}

// check if TTs synthesized by different methods and chosen using 
// different metrics are equal
TEST(ReedMuller, OptimalEqualityTestOn3Vars) { optimalEquality(3); }

TEST(ReedMuller, OptimalEqualityTestOn4Vars) { optimalEquality(4); }

TEST(ReedMuller, OptimalEqualityTestOn5Vars) { optimalEquality(5); }

TEST(ReedMuller, OptimalEqualityTestOn6Vars) { optimalEquality(6); }

TEST(ReedMuller, OptimalEqualityTestOn7Vars) { optimalEquality(7); }

TEST(ReedMuller, OptimalEqualityTestOn8Vars) { optimalEquality(8); }

TEST(ReedMuller, OptimalEqualityTestOn9Vars) { optimalEquality(9); }

TEST(ReedMuller, OptimalEqualityTestOn10Vars) { optimalEquality(10); }

TEST(ReedMuller, OptimalEqualityTestOn11Vars) { optimalEquality(11); }

} // namespace eda::gate::optimizer::synthesis
