//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

#include "gtest/gtest.h"

#include <iostream>
#include <vector>

namespace eda::gate::model {

TEST(SubnetTest, SimpleTest) {
  using Link = Subnet::Link;

  static constexpr size_t Depth  = 3u;
  static constexpr size_t InNum  = (1u << Depth);
  static constexpr size_t OutNum = 1u;

  SubnetBuilder subnetBuilder;

  size_t idx[InNum];
  for (size_t i = 0; i < InNum; ++i) {
    idx[i] = subnetBuilder.addCell(IN);
  }

  for (size_t n = (InNum >> 1); n != 0; n >>= 1) {
    for (size_t i = 0; i < n; ++i) {
      const Link lhs(idx[(i << 1)]);
      const Link rhs(idx[(i << 1) | 1]);

      idx[i] = subnetBuilder.addCell(AND, lhs, rhs);
    }
  }

  subnetBuilder.addCell(OUT, Link(idx[0]));

  subnetBuilder.setInNum(InNum);
  subnetBuilder.setOutNum(OutNum);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  EXPECT_EQ(subnet.getInNum(), InNum);
  EXPECT_EQ(subnet.getOutNum(), OutNum);
  EXPECT_EQ(subnet.size(), 1u << (Depth + 1));

  std::cout << subnet << std::endl;
}

} // namespace eda::gate::model
