//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/cell.h"
#include "gate/model2/celltype.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/resynthesis/reed_muller.h"
#include "util/arith.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer2::resynthesis {

  using DinTruthTable = kitty::dynamic_truth_table;
  using Link = model::Subnet::Link;
  using LinkList = std::vector<Link>;
  using Polynomial = std::vector<uint64_t>;
  using Subnet = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID = model::SubnetID;

  std::string generateRandom(const uint64_t size) {
    std::mt19937 engine;
    std::random_device device;
    engine.seed(device());
    std::string s;
    uint64_t n = 1 << size;
    while (s.size() < n) {
      s += (engine() % 2) + '0';
    }
    return s;
  }

  SubnetID generateTest(const uint64_t numVars) {
    ReedMuller r;
    DinTruthTable t(numVars);
    std::string s = generateRandom(numVars);
    kitty::create_from_binary_string(t, s);
    return r.synthesize(t, -1);
  }

  SubnetID generateSubnetID(std::string s, const uint64_t numVars) {
    ReedMuller r;
    DinTruthTable t(numVars);
    kitty::create_from_binary_string(t, s);
    return r.synthesize(t, -1);
  }

  void testSubnetToSubnet(const Subnet &net, const Subnet &subnet) {
    DinTruthTable t1 = evaluate(net);
    DinTruthTable t2 = evaluate(subnet);
    for (size_t i = 0; i < t1.num_bits(); ++i) {
      ASSERT_EQ(kitty::get_bit(t1, i), kitty::get_bit(t2, i));
    }
  }

  void testSubnetToTruthTable(const uint64_t numVars) {
    ReedMuller r;
    DinTruthTable t(numVars);
    kitty::create_from_binary_string(t, generateRandom(numVars));
    SubnetID subnet = r.synthesize(t, -1);

    DinTruthTable result_table = evaluate(Subnet::get(subnet));
    for (size_t i = 0; i < t.num_bits(); ++i) {
      ASSERT_EQ(kitty::get_bit(t, i), kitty::get_bit(result_table, i));
    }
  }

// We generate a random binary string of length 2^6, 2^10 and 2^14 respectively and see
// if function getTT() on this string works correctly

TEST(ReedMullerModel2, correctTestOnDiffSizes) {
  ReedMuller r;
  std::vector<uint64_t> sizes = {6, 10, 14};
  for (uint32_t j = 0; j < sizes.size(); j++) {

    uint64_t bits = 1 << sizes[j];
    DinTruthTable t(sizes[j]);
    std::string s = generateRandom(sizes[j]);
    kitty::create_from_binary_string(t, s);
    Polynomial f = r.getTT(t);

    for (uint64_t i = 0; i < bits; ++i) {
      ASSERT_EQ(r.apply(f, eda::utils::toBinString(i, sizes[j])),
                  kitty::get_bit(t, i));
    }
  } 
} 

// see if the "00000000" on 3 variables synthesizes a correct function
TEST(ReedMullerModel2, correctTestOnAllZeroes) {
  ReedMuller r;
  DinTruthTable t(3);
  kitty::create_from_binary_string(t, "00000000");
  std::vector<uint64_t> func(9, 0);
  func[8] = 3;
  Polynomial ans = r.getTT(t);
  // should be an empty vector with ans[0] == 3 (num_var);
  ASSERT_EQ(func, ans);
}

// see if the "11111111" on 3 variables synthesizes a correct function
TEST(ReedMullerModel2, correctTestOnAllOnes) {
  ReedMuller r;
  DinTruthTable t(3);
  kitty::create_from_binary_string(t, "11111111");
  Polynomial func(9, 0);
  func[8] = 3;
  func[0] = 1;
  Polynomial ans = r.getTT(t);
  // should be a vector with ans[1] = 3 and ans[0] = 1;
  ASSERT_EQ(func, ans);
}

// Test "sythesize" works correctly (the polynomial is x2 ^ x1x3 ^ x2x3)
TEST(ReedMullerModel2, subnetToSubnetOn3Vars) {
  SubnetID net1 = generateSubnetID("10101100", 3);

  SubnetBuilder builder;
  size_t idx[3];
  for (int i = 0; i < 3; ++i) {
    idx[i] = builder.addCell(model::IN, SubnetBuilder::INPUT);
  }
  LinkList output;
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[1]))));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[1]), Link(idx[2])})));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[0]), Link(idx[2])})));
  size_t out = builder.addCell(model::XOR, output);
  builder.addCell(model::OUT, Link(out), SubnetBuilder::OUTPUT);

  const auto &subnet = Subnet::get(builder.make());
  const auto &net = Subnet::get(net1);
  testSubnetToSubnet(net, subnet);
}

// Tests if "sythesize" works correctly (the polynomial is 1 ^ x1 ^ x1x2 ^ x3 ^ x1x2x3)
TEST(ReedMullerModel2, subnetToSubnetOn3VarsWith1) {
  SubnetID net1 = generateSubnetID("10101101", 3);

  SubnetBuilder builder;
  size_t idx[3];
  for (int i = 0; i < 3; ++i) {
    idx[i] = builder.addCell(model::IN, SubnetBuilder::INPUT);
  }
  LinkList output;
  output.push_back(Link(builder.addCell(model::NOT, Link(idx[0]))));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[0]), Link(idx[1])})));
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[2]))));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[0]), Link(idx[1]), Link(idx[2])})));
  size_t out = builder.addCell(model::XOR, output);
  builder.addCell(model::OUT, Link(out), SubnetBuilder::OUTPUT);
  
  const auto &subnet = Subnet::get(builder.make());
  const auto &net = Subnet::get(net1);
  
  testSubnetToSubnet(net, subnet);
}

// Tests if "sythesize" works correctly (the polynomial is x1 ^ x2 ^ x3 ^ x4 ^ x2x4 ^ x1x2x4 ^ x1x2x3x4)
TEST(ReedMullerModel2, subnetToSubnetOn4Vars) {
  SubnetID net1 = generateSubnetID("1010110110010110", 4);

  SubnetBuilder builder;
  size_t idx[4];
  for (int i = 0; i < 4; ++i) {
    idx[i] = builder.addCell(model::IN, SubnetBuilder::INPUT);
  }
  LinkList output;
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[0]))));
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[1]))));
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[2]))));
  output.push_back(Link(builder.addCell(model::BUF, Link(idx[3]))));
  Link split = Link(builder.addCell(model::XOR, output));
  output.clear();

  output.push_back(Link(builder.addCell(model::AND, {Link(idx[1]), Link(idx[3])})));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[0]), Link(idx[1]), Link(idx[3])})));
  output.push_back(Link(builder.addCell(model::AND, {Link(idx[0]), Link(idx[1]), Link(idx[2]), Link(idx[3])})));
  output.push_back(split);
  
  size_t out = builder.addCell(model::XOR, output);
  builder.addCell(model::OUT, Link(out), SubnetBuilder::OUTPUT);

  const auto &subnet = Subnet::get(builder.make());
  const auto &net = Subnet::get(net1);

  testSubnetToSubnet(net, subnet);
}

// Test if the "synthesize()" method works correctly (it generates Subnet that is equal to the truth table it's based on)
TEST(ReedMullerModel2, subnetToTTOn4Vars) { testSubnetToTruthTable(4); }

TEST(ReedMullerModel2, subnetToTTOn5Vars) { testSubnetToTruthTable(5); }

TEST(ReedMullerModel2, subnetToTTOn6Vars) { testSubnetToTruthTable(6); }

TEST(ReedMullerModel2, subnetToTTOn7Vars) { testSubnetToTruthTable(7); }

TEST(ReedMullerModel2, subnetToTTOn8Vars) { testSubnetToTruthTable(8); }

TEST(ReedMullerModel2, subnetToTTOn9Vars) { testSubnetToTruthTable(9); }

TEST(ReedMullerModel2, subnetToTTOn10Vars) { testSubnetToTruthTable(10); }

// Compare the runtime of the function "synthesize()" on different numbers of variables
TEST(ReedMullerModel2, timeTestOn3Vars) { generateTest(3); }

TEST(ReedMullerModel2, timeTestOn4Vars) { generateTest(4); }

TEST(ReedMullerModel2, timeTestOn5Vars) { generateTest(5); }

TEST(ReedMullerModel2, timeTestOn6Vars) { generateTest(6); }

TEST(ReedMullerModel2, timeTestOn7Vars) { generateTest(7); }

TEST(ReedMullerModel2, timeTestOn8Vars) { generateTest(8); }

TEST(ReedMullerModel2, timeTestOn9Vars) { generateTest(9); }

TEST(ReedMullerModel2, timeTestOn10Vars) { generateTest(10); }

TEST(ReedMullerModel2, timeTestOn11Vars) { generateTest(11); }

TEST(ReedMullerModel2, timeTestOn12Vars) { generateTest(12); }

}//namespace eda::gate::optimizer2::resynthesis
