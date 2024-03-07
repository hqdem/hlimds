//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer2/synthesis/cascade.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

#include <memory>
#include <string>

namespace eda::gate::optimizer2::synthesis {
  
using CNF = std::vector<std::vector<int>>;
using Subnet = model::Subnet;
using TruthTable = kitty::dynamic_truth_table;
  
//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//
TruthTable checkSynthSubnet(int numVars, int bits, model::SubnetID &subnetId) {

  std::vector<TruthTable> result;

  // creating table
  for (int i = 0; i < numVars; i++) {
    TruthTable a(numVars);
    kitty::create_nth_var(a, i);
    result.push_back(a);
  }
  
  const auto &subnet = Subnet::get(subnetId);
  
  auto entries = subnet.getEntries();
  
  for (size_t i = 0; i < entries.size(); i++) {
    auto gate = entries[i];
    if (gate.cell.isAnd()) {
      TruthTable table(numVars);
      table = result[gate.cell.link[0].idx];
      if (gate.cell.link[0].inv) {
        table = ~table;
      }
      for (int j = 1; j < gate.cell.arity; j++) {
        int id;
        id = gate.cell.link[j].idx;
        if (gate.cell.link[j].inv) {
          table &= (~result[id]);
        } else {
          table &= result[id];
        }
      }
      result.push_back(table);
    }
    else if (gate.cell.isOr()) {
      TruthTable table(numVars);
      table = result[gate.cell.link[0].idx];
      if (gate.cell.link[0].inv) {
        table = ~table;
      }
      for (int j = 1; j < gate.cell.arity; j++) {
        int id;
        id = gate.cell.link[j].idx;
        if (gate.cell.link[j].inv) {
          table |= (~result[id]);
        } else {
          table |= result[id];
        }
      }
      result.push_back(table);
    }
    
  }
  return result[result.size() - 1];
}

void subnetEquivalenceTest(int vars) {
  TruthTable table(vars);
  kitty::create_random(table);

  CascadeSynthesizer resynth;
  const auto subnetId = resynth.synthesize(table);
  auto subnetIdNew = subnetId;
  
  TruthTable tableSubnet = checkSynthSubnet(vars, 1<<vars, subnetIdNew); 

  EXPECT_TRUE(table == tableSubnet);
}

TEST(Cascade, SubnetEquivalenceTest3) {
  subnetEquivalenceTest(3);
}

TEST(Cascade, SubnetEquivalenceTest4) {
  subnetEquivalenceTest(4);
}

TEST(Cascade, SubnetEquivalenceTest5) {
  subnetEquivalenceTest(5);
}

TEST(Cascade, SubnetEquivalenceTest6) {
  subnetEquivalenceTest(6);
}

TEST(Cascade, SubnetEquivalenceTest7) {
  subnetEquivalenceTest(7);
}

TEST(Cascade, SubnetEquivalenceTest8) {
  subnetEquivalenceTest(8);
}

TEST(Cascade, SubnetEquivalenceTest9) {
  subnetEquivalenceTest(9);
}

TEST(Cascade, SubnetEquivalenceTest10) {
  subnetEquivalenceTest(10);
}

TEST(Cascade, SubnetTest) {
  int vars = 2;
  TruthTable table(vars);
  kitty::create_from_binary_string(table, "1000");

  CascadeSynthesizer resynth;
  const auto subnetId = resynth.synthesize(table);
  const auto &subnet = Subnet::get(subnetId);

  EXPECT_TRUE(subnet.size() == 4);
}

TEST(Cascade, MaxArityTest) {
  int vars = 3;
  int maxArity = 3;
  TruthTable table(vars);
  kitty::create_from_binary_string(table, "10000110");

  CascadeSynthesizer resynth;
  const auto subnetId = resynth.synthesize(table, maxArity);
  const auto &subnet = Subnet::get(subnetId);
   
  auto subnetIdNew = subnetId;
  TruthTable tableSubnet = checkSynthSubnet(vars, 1<<vars, subnetIdNew); 
  
  bool check = true;
  auto entries = subnet.getEntries();
  for (uint64_t i = 0; i < entries.size(); i++) {
    if (entries[i].cell.arity > maxArity) {
      check = false;
      break;
    }
  }

  EXPECT_TRUE(check && tableSubnet == table);
} 
}; // namespace eda::gate::optimizer2::synthesis
