//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/optimizer2/pass.h"

#include "gtest/gtest.h"

#include <iostream>

namespace eda::gate::optimizer2 {

static void initSubnetBuilder(model::SubnetBuilder &builder) {
  const auto &links = builder.addInputs(8);

  const auto &res01 = builder.addCell(model::XOR, links[0], links[1]);
  const auto &res12 = builder.addCell(model::XOR, links[1], links[2]);
  const auto &res23 = builder.addCell(model::XOR, links[2], links[3]);
  const auto &res34 = builder.addCell(model::XOR, links[3], links[4]);
  const auto &res45 = builder.addCell(model::XOR, links[4], links[5]);
  const auto &res56 = builder.addCell(model::XOR, links[5], links[6]);
  const auto &res67 = builder.addCell(model::XOR, links[6], links[7]);
  const auto &res70 = builder.addCell(model::XOR, links[7], links[0]);
  const auto &res02 = builder.addCell(model::AND, res01, res12);
  const auto &res24 = builder.addCell(model::AND, res23, res34);
  const auto &res46 = builder.addCell(model::AND, res45, res56);
  const auto &res60 = builder.addCell(model::AND, res67, res70);
  const auto &res04 = builder.addCell(model::XOR, res02, res24);
  const auto &res40 = builder.addCell(model::XOR, res46, res60);
  const auto &res00 = builder.addCell(model::AND, res04, res40);

  builder.addOutput(res00);
}

TEST(Pass, Rw) {
  model::SubnetBuilder builder;
  initSubnetBuilder(builder);

  const auto &pass = rw();
  pass.transform(builder);

  const auto subnetID = builder.make();
  std::cout << model::Subnet::get(subnetID) << std::endl;
}

TEST(Pass, Rwz) {
  model::SubnetBuilder builder;
  initSubnetBuilder(builder);

  const auto &pass = rwz();
  pass.transform(builder);

  const auto subnetID = builder.make();
  std::cout << model::Subnet::get(subnetID) << std::endl;
}

} // namespace eda::gate::optimizer2
