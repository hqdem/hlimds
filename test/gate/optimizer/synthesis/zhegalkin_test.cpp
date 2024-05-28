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

  std::string generateRandom(const uint64_t size) {
    std::mt19937 engine(0);
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
    Zhegalkin r;
    DinTruthTable t(numVars);
    std::string s = generateRandom(numVars);
    kitty::create_from_binary_string(t, s);
    return r.synthesize(t);
  }

  SubnetID generateSubnetID(std::string s, const uint64_t numVars) {
    Zhegalkin r;
    DinTruthTable t(numVars);
    kitty::create_from_binary_string(t, s);
    return r.synthesize(t);
  }

  void testSubnetToSubnet(const Subnet &net, const Subnet &subnet) {
    DinTruthTable t1 = evaluateSingleOut(net);
    DinTruthTable t2 = evaluateSingleOut(subnet);
    ASSERT_TRUE(t1 == t2);
  }

  void testSubnetToTruthTable(const uint64_t numVars) {
    Zhegalkin r;
    DinTruthTable t(numVars);
    kitty::create_from_binary_string(t, generateRandom(numVars));
    const auto &subnet = Subnet::get(r.synthesize(t));

    ASSERT_TRUE(eda::gate::model::utils::equalTruthTables(subnet, t));
  }

  void subnetToSubnetDiffArity(const uint64_t numVars) {
    Zhegalkin r;
    DinTruthTable t(numVars);
    kitty::create_from_binary_string(t, generateRandom(numVars));
    SubnetID baseSubnet = r.synthesize(t);
    DinTruthTable baseTable = evaluateSingleOut(Subnet::get(baseSubnet));

    for (uint16_t arity = 3; arity < Subnet::Cell::InPlaceLinks + 1; ++arity) {
      const auto &subnet = Subnet::get(r.synthesize(t, arity));
      ASSERT_TRUE(eda::gate::model::utils::checkArity(subnet, arity));
      ASSERT_TRUE(eda::gate::model::utils::equalTruthTables(subnet, baseTable));
    }
  }

// We generate a random binary string of length 2^6, 2^10 and 2^14 respectively and see
// if function getTT() on this string works correctly
TEST(Zhegalkin, correctTestOnDiffSizes) {
  Zhegalkin r;
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
TEST(Zhegalkin, correctTestOnAllZeroes) {
  Zhegalkin r;
  DinTruthTable t(3);
  kitty::create_from_binary_string(t, "00000000");
  std::vector<uint64_t> func(9, 0);
  func[8] = 3;
  Polynomial ans = r.getTT(t);
  // should be an empty vector with ans[0] == 3 (num_var);
  ASSERT_EQ(func, ans);
}

// see if the "11111111" on 3 variables synthesizes a correct function
TEST(Zhegalkin, correctTestOnAllOnes) {
  Zhegalkin r;
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
TEST(Zhegalkin, subnetToSubnetOn3Vars) {
  SubnetID net1 = generateSubnetID("10101100", 3);

  SubnetBuilder builder;

  const auto input = builder.addInputs(3);

  LinkList output;
  output.push_back(input[1]);
  output.push_back(builder.addCell(model::AND, input[1], input[2]));
  output.push_back(builder.addCell(model::AND, input[0], input[2]));

  builder.addOutput(builder.addCell(model::XOR, output));

  const auto &subnet = Subnet::get(builder.make());
  const auto &net = Subnet::get(net1);

  testSubnetToSubnet(net, subnet);
}

// Tests if "sythesize" works correctly (the polynomial is x1 ^ x2 ^ x3 ^ x4 ^ x2x4 ^ x1x2x4 ^ x1x2x3x4)
TEST(Zhegalkin, subnetToSubnetOn4Vars) {
  SubnetID net1 = generateSubnetID("1010110110010110", 4);

  SubnetBuilder builder;

  const auto input = builder.addInputs(4);
  const auto split = builder.addCell(model::XOR, input);

  LinkList output;
  output.push_back(builder.addCell(model::AND, input[1], input[3]));
  output.push_back(builder.addCell(model::AND, input[0], input[1], input[3]));
  output.push_back(builder.addCell(model::AND, input[0], input[1], 
    input[2], input[3]));

  output.push_back(split);
  
  builder.addOutput(builder.addCell(model::XOR, output));

  const auto &subnet = Subnet::get(builder.make());
  const auto &net = Subnet::get(net1);

  testSubnetToSubnet(net, subnet);
}

// Test if the "synthesize()" method works correctly 
// (it generates Subnet that is equal to the truth table it's based on)
TEST(Zhegalkin, subnetToTTOn2Vars) { testSubnetToTruthTable(2); }

TEST(Zhegalkin, subnetToTTOn4Vars) { testSubnetToTruthTable(4); }

TEST(Zhegalkin, subnetToTTOn5Vars) { testSubnetToTruthTable(5); }

TEST(Zhegalkin, subnetToTTOn6Vars) { testSubnetToTruthTable(6); }

TEST(Zhegalkin, subnetToTTOn7Vars) { testSubnetToTruthTable(7); }

TEST(Zhegalkin, subnetToTTOn8Vars) { testSubnetToTruthTable(8); }

TEST(Zhegalkin, subnetToTTOn9Vars) { testSubnetToTruthTable(9); }

TEST(Zhegalkin, subnetToTTOn10Vars) { testSubnetToTruthTable(10); }

// Compare the runtime of the function "synthesize()" on different numbers of variables
TEST(Zhegalkin, timeTestOn3Vars) { generateTest(3); }

TEST(Zhegalkin, timeTestOn4Vars) { generateTest(4); }

TEST(Zhegalkin, timeTestOn5Vars) { generateTest(5); }

TEST(Zhegalkin, timeTestOn6Vars) { generateTest(6); }

TEST(Zhegalkin, timeTestOn7Vars) { generateTest(7); }

TEST(Zhegalkin, timeTestOn8Vars) { generateTest(8); }

TEST(Zhegalkin, timeTestOn9Vars) { generateTest(9); }

TEST(Zhegalkin, timeTestOn10Vars) { generateTest(10); }

// Compare subnets generated on the same DinTruthTable but with different
// maxArity values to see if they are equal to each other
TEST(Zhegalkin, DiffArityOn4Values) { subnetToSubnetDiffArity(4); }

TEST(Zhegalkin, DiffArityOn5Values) { subnetToSubnetDiffArity(5); }

TEST(Zhegalkin, DiffArityOn6Values) { subnetToSubnetDiffArity(6); }

TEST(Zhegalkin, DiffArityOn7Values) { subnetToSubnetDiffArity(7); }

TEST(Zhegalkin, DiffArityOn8Values) { subnetToSubnetDiffArity(8); }

TEST(Zhegalkin, DiffArityOn9Values) { subnetToSubnetDiffArity(9); }

TEST(Zhegalkin, DiffArityOn10Values) { subnetToSubnetDiffArity(10); }
} // namespace eda::gate::optimizer::synthesis
