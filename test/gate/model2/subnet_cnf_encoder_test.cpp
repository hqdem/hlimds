//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_cnf_encoder.h"

#include "gtest/gtest.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// SAT subnets
//===----------------------------------------------------------------------===//

// Implements y = ~(x & ~x).
static SubnetID makeOneAndSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(AND, x, ~x);
  builder.addOutput(~y);
  return builder.make();
}

// Implements y = (x | ~x).
static SubnetID makeOneOrSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(OR, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = (x ^ ~x).
static SubnetID makeOneXorSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(XOR, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = maj(x, ~x, 1).
static SubnetID makeOneMajSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto o = builder.addCell(ONE);
  const auto y = builder.addCell(MAJ, x, ~x, o);
  builder.addOutput(y);
  return builder.make();
}

//===----------------------------------------------------------------------===//
// UNSAT subnets
//===----------------------------------------------------------------------===//

// Implements y = (x & ~x).
static SubnetID makeZeroAndSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(AND, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = ~(x | ~x).
static SubnetID makeZeroOrSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(OR, x, ~x);
  builder.addOutput(~y);
  return builder.make();
}

// Implements y = (x ^ x).
static SubnetID makeZeroXorSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(XOR, x, x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = maj(x, ~x, 0).
static SubnetID makeZeroMajSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto z = builder.addCell(ZERO);
  const auto y = builder.addCell(MAJ, x, ~x, z);
  builder.addOutput(y);
  return builder.make();
}

//===----------------------------------------------------------------------===//
// Tests
//===----------------------------------------------------------------------===//

inline bool checkAlways(SubnetID subnetID, bool value) {
  const auto &subnet = Subnet::get(subnetID);
  const auto &encoder = SubnetEncoder::get();

  eda::gate::solver::Solver solver;
  SubnetEncoderContext context(subnet, solver);

  encoder.encode(subnet, context, solver);
  encoder.encodeEqual(subnet, context, solver, subnet.getOut(0), !value);

  // The opposite is impossible.
  return !solver.solve();
}

TEST(SubnetCnfEncoderTest, OneAndTest) {
  EXPECT_TRUE(checkAlways(makeOneAndSubnet(), 1));
}

TEST(SubnetCnfEncoderTest, OneOrTest) {
  EXPECT_TRUE(checkAlways(makeOneOrSubnet(), 1));
}

TEST(SubnetCnfEncoderTest, OneXorTest) {
  EXPECT_TRUE(checkAlways(makeOneXorSubnet(), 1));
}

TEST(SubnetCnfEncoderTest, OneMajTest) {
  EXPECT_TRUE(checkAlways(makeOneMajSubnet(), 1));
}

TEST(SubnetCnfEncoderTest, ZeroAndTest) {
  EXPECT_TRUE(checkAlways(makeZeroAndSubnet(), 0));
}

TEST(SubnetCnfEncoderTest, ZeroOrTest) {
  EXPECT_TRUE(checkAlways(makeZeroOrSubnet(), 0));
}

TEST(SubnetCnfEncoderTest, ZeroXorTest) {
  EXPECT_TRUE(checkAlways(makeZeroXorSubnet(), 0));
}

TEST(SubnetCnfEncoderTest, ZeroMajTest) {
  EXPECT_TRUE(checkAlways(makeZeroMajSubnet(), 0));
}

} // namespace eda::gate::model
