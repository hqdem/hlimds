//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/resynthesis/reed_muller.h"
#include "gtest/gtest.h"

namespace eda::gate::optimizer::resynthesis {

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

  void generateTest(const uint64_t size) {
    ReedMuller r;
    kitty::dynamic_truth_table t(size);
    std::string s = generateRandom(size);
    kitty::create_from_binary_string(t, s);
    r.getGNet(t);
  }

// Testing if synthesis function works correctly
// We generate a random binary string of length 2^6, 2^10 and 2^14 respectively

TEST(ReedMuller, correctTestOnDiffSizes) {
  ReedMuller r;
  std::vector<uint64_t> sizes = {6, 10, 14};
  for (uint32_t j = 0; j < sizes.size(); j++) {

    uint64_t bits = 1 << sizes[j];
    DinTruthTable t(sizes[j]);
    std::string s = generateRandom(sizes[j]);
    kitty::create_from_binary_string(t, s);
    Polynomial f = r.getTT(t);
    
    for (uint64_t i = 0; i < bits; ++i) {
      ASSERT_EQ(r.apply(f, r.toBinString(i, sizes[j])),
                  kitty::get_bit(t, i));
    }
  } 
} 

// see if the "00000000" on 3 variables synthesizes a correct function
TEST(ReedMuller, correctTestOnAllZeroes) {
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
TEST(ReedMuller, correctTestOnAllOnes) {
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

// compare the behaviour of the function "getGNet" on different
// numbers of variables
TEST(ReedMuller, timeTestOn3Vars) { generateTest(3); }

TEST(ReedMuller, timeTestOn4Vars) { generateTest(4); }

TEST(ReedMuller, timeTestOn5Vars) { generateTest(5); }

TEST(ReedMuller, timeTestOn6Vars) { generateTest(6); }

TEST(ReedMuller, timeTestOn7Vars) { generateTest(7); }

TEST(ReedMuller, timeTestOn8Vars) { generateTest(8); }

TEST(ReedMuller, timeTestOn9Vars) { generateTest(9); }

TEST(ReedMuller, timeTestOn10Vars) { generateTest(10); }

TEST(ReedMuller, timeTestOn11Vars) { generateTest(11); }

TEST(ReedMuller, timeTestOn12Vars) { generateTest(12); }

TEST(ReedMuller, timeTestOn13Vars) { generateTest(13); }

TEST(ReedMuller, timeTestOn14Vars) { generateTest(14); }

TEST(ReedMuller, timeTestOn15Vars) { generateTest(15); }

TEST(ReedMuller, timeTestOn16Vars) { generateTest(16); }
}
