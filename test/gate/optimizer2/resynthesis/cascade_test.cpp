//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer2/resynthesis/cascade.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

#include <memory>
#include <string>

namespace eda::gate::optimizer2::resynthesis {
  
using CNF = std::vector<std::vector<int>>;
using Subnet = model::Subnet;
using TruthTable = kitty::dynamic_truth_table;
  
//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//
  
/// Transforms output into string
TruthTable checkSynth(int numVars, int bits, CNF &output) {

  int numOnes = bits, numOnesPrev = bits; // creating table

  std::vector<TruthTable> result;

  for (int i = 0; i < 2; i++) {
    TruthTable a(numVars);
    result.push_back(a);
    for (int j = 0; j < bits; j++) {
      if (!i) {
        clear_bit(result[i], j);
      } else {
        set_bit(result[i], j);
      }
    }
  }
  for (int i = 2; i < numVars + 2; i++) {
    TruthTable a(numVars);
    result.push_back(a);
    numOnes /= 2;

    for (int j = 0; j < bits; j++) {
      if ((j % numOnesPrev) < numOnes) {
        set_bit(result[i], j);
      } else {
        clear_bit(result[i], j);
      }
    }
    numOnesPrev = numOnes;
  }
  
  numOnes = bits, numOnesPrev = bits;
  for (int i = numVars + 2; i < numVars * 2 + 2; i++) {
    TruthTable a(numVars);
    result.push_back(a);
    numOnes /= 2;

    for (int j = 0; j < bits; j++) {
      if ((j % numOnesPrev) < numOnes) {
        clear_bit(result[i], j);
      } else {
        set_bit(result[i], j);
      }
    }
    numOnesPrev = numOnes;
  }

  for (long unsigned int i = numVars * 2 + 2; i < output[0].size(); i++) {
    if (!output[1][i] && !output[2][i]) { // constant
      int id = output[0][i];
      result.push_back(result[id]);
    } else if (output[0][i] == 2) { // &
      int id1 = output[1][i];
      int id2 = output[2][i];
      TruthTable tt = result[id1];
      tt &= result[id2];
      result.push_back(tt);
    } else if (output[0][i] == 3) { // |
      int id1 = output[1][i];
      int id2 = output[2][i];
      TruthTable tt = result[id1];
      tt |= result[id2];
      result.push_back(tt);
    }
  }
  return result[result.size() - 1];
}

/// Checks if input string and output are equal
void synthTest(int vars, std::string str) {
  TruthTable table(vars);
  kitty::create_from_binary_string(table, str);

  Cascade resynth;
  CNF form = resynth.normalForm(table);
  std::vector<int> values;
  CNF output = resynth.getFunction(table, form, values);
  TruthTable tt(vars);
  tt = checkSynth(vars, table.num_bits(), output);
    
  EXPECT_TRUE(tt == table);
}

TEST(Cascade, CorrectSynthTest) {
  synthTest(4, "1001000000100100");
}

TEST(Cascade, SubnetTest) {
  int vars = 2;
  TruthTable table(vars);
  kitty::create_from_binary_string(table, "1000");

  Cascade resynth;
  const auto subnetId = resynth.synthesize(table, -1);
  const auto &subnet = Subnet::get(subnetId);

  EXPECT_TRUE(subnet.size() == 6);
}

TEST(Cascade, MaxArityTest) {
  int vars = 3;
  int maxArity = 3;
  TruthTable table(vars);
  kitty::create_from_binary_string(table, "10000110");

  synthTest(vars, "10000110");

  Cascade resynth;
  const auto subnetId = resynth.synthesize(table, maxArity);
  const auto &subnet = Subnet::get(subnetId);
  bool check = true;
  auto entries = subnet.getEntries();
  for (uint64_t i = 0; i < entries.size(); i++) {
    if (entries[i].cell.arity > maxArity) {
      check = false;
      break;
    }
  }

  EXPECT_TRUE(check);
} 
}; // namespace eda::gate::optimizer2::resynthesis
