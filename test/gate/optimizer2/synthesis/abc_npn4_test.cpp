//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_checking.h"
#include "gate/optimizer2/npn.h"
#include "gate/optimizer2/synthesis/abc_npn4.h"

#include "gtest/gtest.h"

#include <array>
#include <iostream>
#include <sstream>
#include <string>

using Synthesizer = eda::gate::optimizer2::AbcNpn4Synthesizer;

inline void testHexString(uint16_t k, const std::string &hex) {
  Synthesizer::TruthTable func(k);
  kitty::create_from_hex_string(func, hex);

  const auto subnetID = Synthesizer::get().synthesize(func);

  if (subnetID != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = eda::gate::model::Subnet::get(subnetID);

#ifdef UTOPIA_DEBUG
    std::cout << subnet << std::endl;
#endif // UTOPIA_DEBUG

    if (subnet.getInNum() > 0) {
      EXPECT_TRUE(eda::gate::model::utils::equalTruthTables(subnet, func));
    }
  }
}

TEST(AbcNpn4Test1, ZeroTest) {
  testHexString(1, "0");
}

TEST(AbcNpn4Test1, OneTest) {
  testHexString(1, "3");
}

TEST(AbcNpn4Test1, Var0Test) {
  testHexString(1, "2");
}

TEST(AbcNpn4Test1, Not0Test) {
  testHexString(1, "1");
}

TEST(AbcNpn4Test2, ZeroTest) {
  testHexString(2, "0");
}

TEST(AbcNpn4Test2, OneTest) {
  testHexString(2, "f");
}

TEST(AbcNpn4Test2, Var0Test) {
  testHexString(2, "a");
}

TEST(AbcNpn4Test2, Var1Test) {
  testHexString(2, "c");
}

TEST(AbcNpn4Test2, Not0Test) {
  testHexString(2, "5");
}

TEST(AbcNpn4Test2, Not1Test) {
  testHexString(2, "3");
}

TEST(AbcNpn4Test2, And01Test) {
  testHexString(2, "8");
}

TEST(AbcNpn4Test2, Xor01Test) {
  testHexString(2, "6");
}

TEST(AbcNpn4Test3, ZeroTest) {
  testHexString(3, "00");
}

TEST(AbcNpn4Test3, OneTest) {
  testHexString(3, "ff");
}

TEST(AbcNpn4Test3, Var0Test) {
  testHexString(3, "aa");
}

TEST(AbcNpn4Test3, Var1Test) {
  testHexString(3, "cc");
}

TEST(AbcNpn4Test3, Var2Test) {
  testHexString(3, "f0");
}

TEST(AbcNpn4Test3, Not0Test) {
  testHexString(3, "55");
}

TEST(AbcNpn4Test3, Not1Test) {
  testHexString(3, "33");
}

TEST(AbcNpn4Test3, Not2Test) {
  testHexString(3, "0f");
}

TEST(AbcNpn4Test3, And01Test) {
  testHexString(3, "88");
}

TEST(AbcNpn4Test3, And12Test) {
  testHexString(3, "c0");
}

TEST(AbcNpn4Test3, And012Test) {
  testHexString(3, "80");
}

TEST(AbcNpn4Test3, Xor012Test) {
  testHexString(3, "69");
}

TEST(AbcNpn4Test4, ZeroTest) {
  testHexString(4, "0000");
}

TEST(AbcNpn4Test4, OneTest) {
  testHexString(4, "ffff");
}

TEST(AbcNpn4Test4, Var0Test) {
  testHexString(4, "aaaa");
}

TEST(AbcNpn4Test4, Var1Test) {
  testHexString(4, "cccc");
}

TEST(AbcNpn4Test4, Var2Test) {
  testHexString(4, "f0f0");
}

TEST(AbcNpn4Test4, Var3Test) {
  testHexString(4, "ff00");
}

TEST(AbcNpn4Test4, Not0Test) {
  testHexString(4, "5555");
}

TEST(AbcNpn4Test4, Not1Test) {
  testHexString(4, "3333");
}

TEST(AbcNpn4Test4, Not2Test) {
  testHexString(4, "0f0f");
}

TEST(AbcNpn4Test4, Not3Test) {
  testHexString(4, "00ff");
}

TEST(AbcNpn4Test4, And01Test) {
  testHexString(4, "8888");
}

TEST(AbcNpn4Test4, And12Test) {
  testHexString(4, "c0c0");
}

TEST(AbcNpn4Test4, And23Test) {
  testHexString(4, "f000");
}

TEST(AbcNpn4Test4, And012Test) {
  testHexString(4, "8080");
}

TEST(AbcNpn4Test4, And123Test) {
  testHexString(4, "c000");
}

TEST(AbcNpn4Test4, And0123Test) {
  testHexString(4, "8000");
}

TEST(AbcNpn4Test4, And0n3Test) {
  testHexString(4, "00aa");
}

TEST(AbcNpn4Test4, Xor0123Test) {
  testHexString(4, "6996");
}

TEST(AbcNpn4Test4, RandTest) {
  testHexString(4, "5f6e");
}

TEST(AbcNpn4Test, AllNpn4Test) {
  constexpr size_t k = 4;
  constexpr size_t n = 1 << (1 << k);
  constexpr size_t w = (1 << k) >> 2;

  size_t count{0};
  std::array<bool, n> covered{false};

  for (size_t i = 0; i < eda::gate::optimizer2::npn4Num; ++i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(w)
           << std::hex << eda::gate::optimizer2::npn4[i];

    const auto hex = stream.str();

    Synthesizer::TruthTable func(k);
    kitty::create_from_hex_string(func, hex);

    const auto npnCanon = kitty::exact_npn_canonization(func);
    const auto npnTable = static_cast<uint16_t>(*std::get<0>(npnCanon).begin());

    if (covered[npnTable]) {
      continue;
    }
    covered[npnTable] = true;

    const auto subnetID = Synthesizer::get().synthesize(func);

#ifdef UTOPIA_DEBUG
    std::cout << std::setfill('0') << std::setw(w)
              << std::hex << npnTable << std::endl;
#endif // UTOPIA_DEBUG

    if (subnetID != eda::gate::model::OBJ_NULL_ID) {
#ifdef UTOPIA_DEBUG
      const auto &subnet = eda::gate::model::Subnet::get(subnetID);
      std::cout << std::dec << subnet << std::endl;
#endif // UTOPIA_DEBUG
      count++;
    } else {
#ifdef UTOPIA_DEBUG
      std::cout << "Unsupported" << std::endl << std::endl;
#endif // UTOPIA_DEBUG
    }
  }

  EXPECT_EQ(count, 135);
}
