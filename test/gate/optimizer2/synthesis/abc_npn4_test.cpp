//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_checking.h"
#include "gate/optimizer2/synthesis/abc_npn4.h"

#include "gtest/gtest.h"

#include <iostream>
#include <string>

using Synthesizer = eda::gate::optimizer2::AbcNpn4Synthesizer;

inline void testHexString(uint16_t k, const std::string &hex) {
  Synthesizer::TruthTable func(k);
  kitty::create_from_hex_string(func, hex);

  const auto subnetID = Synthesizer::get().synthesize(func);

  if (subnetID != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = eda::gate::model::Subnet::get(subnetID);
    std::cout << subnet << std::endl;

    if (subnet.getInNum() > 0) { // FIXME:
      // EXPECT_TRUE(eda::gate::model::utils::equalTruthTables(subnet, func)); // FIXME:
    }
  }
}

TEST(AbcNpn4Test, ZeroTest) {
  testHexString(4, "0000");
}

TEST(AbcNpn4Test, OneTest) {
  testHexString(4, "ffff");
}

TEST(AbcNpn4Test, X0Test) {
  testHexString(4, "aaaa");
}

TEST(AbcNpn4Test, X1Test) {
  testHexString(4, "cccc");
}

TEST(AbcNpn4Test, X2Test) {
  testHexString(4, "f0f0");
}

TEST(AbcNpn4Test, X3Test) {
  testHexString(4, "ff00");
}

TEST(AbcNpn4Test, NotX0Test) {
  testHexString(4, "5555");
}

TEST(AbcNpn4Test, NotX1Test) {
  testHexString(4, "3333");
}

TEST(AbcNpn4Test, NotX2Test) {
  testHexString(4, "0f0f");
}

TEST(AbcNpn4Test, NotX3Test) {
  testHexString(4, "00ff");
}
