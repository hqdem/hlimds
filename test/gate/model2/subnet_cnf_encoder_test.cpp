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
static SubnetID makeSatAndSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(AND, x, ~x);
  builder.addOutput(~y);
  return builder.make();
}

// Implements y = (x | ~x).
static SubnetID makeSatOrSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(OR, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = (x ^ ~x).
static SubnetID makeSatXorSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(XOR, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = maj(x, ~x, 1).
static SubnetID makeSatMajSubnet() {
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
static SubnetID makeUnsatAndSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(AND, x, ~x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = ~(x | ~x).
static SubnetID makeUnsatOrSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(OR, x, ~x);
  builder.addOutput(~y);
  return builder.make();
}

// Implements y = (x ^ x).
static SubnetID makeUnsatXorSubnet() {
  SubnetBuilder builder;
  const auto x = builder.addInput();
  const auto y = builder.addCell(XOR, x, x);
  builder.addOutput(y);
  return builder.make();
}

// Implements y = maj(x, ~x, 0).
static SubnetID makeUnsatMajSubnet() {
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

inline bool isSat(SubnetID subnetID) {
  eda::gate::solver::Solver solver;
  const auto &subnet = Subnet::get(subnetID);

  SubnetEncoder::get().encode(subnet, solver);
  solver.dump("cnf.dimacs");
  return solver.solve();
}

TEST(SubnetCnfEncoderTest, SatAndTest) {
  EXPECT_TRUE(isSat(makeSatAndSubnet()));
}

TEST(SubnetCnfEncoderTest, SatOrTest) {
  EXPECT_TRUE(isSat(makeSatOrSubnet()));
}

TEST(SubnetCnfEncoderTest, SatXorTest) {
  EXPECT_TRUE(isSat(makeSatXorSubnet()));
}

TEST(SubnetCnfEncoderTest, SatMajTest) {
  EXPECT_TRUE(isSat(makeSatMajSubnet()));
}

TEST(SubnetCnfEncoderTest, UnsatAndTest) {
  EXPECT_FALSE(isSat(makeUnsatAndSubnet()));
}

TEST(SubnetCnfEncoderTest, UnsatOrTest) {
  EXPECT_FALSE(isSat(makeUnsatOrSubnet()));
}

TEST(SubnetCnfEncoderTest, UnsatXorTest) {
  EXPECT_FALSE(isSat(makeUnsatXorSubnet()));
}

TEST(SubnetCnfEncoderTest, UnsatMajTest) {
  EXPECT_FALSE(isSat(makeUnsatMajSubnet()));
}

} // namespace eda::gate::model
