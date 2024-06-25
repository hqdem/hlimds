//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/verifier.h"
#include "gate/model/utils/subnet_cnf_encoder.h"

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

/// Eventually SAT subnets

static SubnetID makeOneZeroOrSubnet() {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto y = builder.addCell(OR, inputs[0], inputs[1]);
  builder.addOutput(y);
  return builder.make();
}

//===----------------------------------------------------------------------===//
// Tests
//===----------------------------------------------------------------------===//

inline bool check(SubnetID subnetID, bool linkVal, bool inv, bool always) {
  const auto &subnet = Subnet::get(subnetID);

  eda::gate::solver::Solver solver;
  debugger::Verifier verifier(subnet, solver);

  auto propID = verifier.makeEqualty(subnet.getOut(0), linkVal);
  return always ?
    verifier.checkAlways(verifier.getProperty(propID), inv) :
    verifier.checkEventually(verifier.getProperty(propID), inv);
}

inline bool checkEventually(SubnetID subnetID, bool linkVal, bool inv) {
  return check(subnetID, linkVal, inv, false);
}

inline bool checkAlways(SubnetID subnetID, bool linkVal, bool inv) {
  return check(subnetID, linkVal, inv, true);
}

TEST(VerifierTest, OneAndTest) {
  EXPECT_FALSE(checkAlways(makeOneAndSubnet(), 0, 0));
  EXPECT_TRUE(checkAlways(makeOneAndSubnet(), 1, 0));

  EXPECT_FALSE(checkEventually(makeOneAndSubnet(), 0, 0));
  EXPECT_TRUE(checkEventually(makeOneAndSubnet(), 1, 0));
}

TEST(VerifierTest, OneOrTest) {
  EXPECT_FALSE(checkAlways(makeOneOrSubnet(), 0, 0));
  EXPECT_TRUE(checkAlways(makeOneOrSubnet(), 1, 0));

  EXPECT_FALSE(checkEventually(makeOneOrSubnet(), 0, 0));
  EXPECT_TRUE(checkEventually(makeOneOrSubnet(), 1, 0));
}

TEST(VerifierTest, OneXorTest) {
  EXPECT_FALSE(checkAlways(makeOneXorSubnet(), 0, 0));
  EXPECT_TRUE(checkAlways(makeOneXorSubnet(), 1, 0));

  EXPECT_FALSE(checkEventually(makeOneXorSubnet(), 0, 0));
  EXPECT_TRUE(checkEventually(makeOneXorSubnet(), 1, 0));
}

TEST(VerifierTest, OneMajTest) {
  EXPECT_FALSE(checkAlways(makeOneMajSubnet(), 0, 0));
  EXPECT_TRUE(checkAlways(makeOneMajSubnet(), 1, 0));

  EXPECT_FALSE(checkEventually(makeOneMajSubnet(), 0, 0));
  EXPECT_TRUE(checkEventually(makeOneMajSubnet(), 1, 0));
}

TEST(VerifierTest, ZeroAndTest) {
  EXPECT_TRUE(checkAlways(makeZeroAndSubnet(), 0, 0));
  EXPECT_FALSE(checkAlways(makeZeroAndSubnet(), 1, 0));

  EXPECT_TRUE(checkEventually(makeZeroAndSubnet(), 0, 0));
  EXPECT_FALSE(checkEventually(makeZeroAndSubnet(), 1, 0));
}

TEST(VerifierTest, ZeroOrTest) {
  EXPECT_TRUE(checkAlways(makeZeroOrSubnet(), 0, 0));
  EXPECT_FALSE(checkAlways(makeZeroOrSubnet(), 1, 0));

  EXPECT_TRUE(checkEventually(makeZeroOrSubnet(), 0, 0));
  EXPECT_FALSE(checkEventually(makeZeroOrSubnet(), 1, 0));
}

TEST(VerifierTest, ZeroXorTest) {
  EXPECT_TRUE(checkAlways(makeZeroXorSubnet(), 0, 0));
  EXPECT_FALSE(checkAlways(makeZeroXorSubnet(), 1, 0));

  EXPECT_TRUE(checkEventually(makeZeroXorSubnet(), 0, 0));
  EXPECT_FALSE(checkEventually(makeZeroXorSubnet(), 1, 0));
}

TEST(VerifierTest, ZeroMajTest) {
  EXPECT_TRUE(checkAlways(makeZeroMajSubnet(), 0, 0));
  EXPECT_FALSE(checkAlways(makeZeroMajSubnet(), 1, 0));

  EXPECT_TRUE(checkEventually(makeZeroMajSubnet(), 0, 0));
  EXPECT_FALSE(checkEventually(makeZeroMajSubnet(), 1, 0));
}

TEST(VerifierTest, OneZeroOrTest) {
  EXPECT_FALSE(checkAlways(makeOneZeroOrSubnet(), 0, 0));
  EXPECT_FALSE(checkAlways(makeOneZeroOrSubnet(), 1, 0));

  EXPECT_TRUE(checkEventually(makeOneZeroOrSubnet(), 0, 0));
  EXPECT_TRUE(checkEventually(makeOneZeroOrSubnet(), 1, 0));
}

TEST(VerifierTest, InvProperty) {
  EXPECT_FALSE(checkAlways(makeZeroMajSubnet(), 0, 1));
  EXPECT_TRUE(checkAlways(makeZeroMajSubnet(), 1, 1));

  EXPECT_FALSE(checkEventually(makeZeroMajSubnet(), 0, 1));
  EXPECT_TRUE(checkEventually(makeZeroMajSubnet(), 1, 1));
}

TEST(VerifierTest, SeveralPropsTest) {
  SubnetBuilder builder;
  const auto &inputs =  builder.addInputs(3);
  const auto &andLink0 = builder.addCell(AND, inputs[0], ~inputs[0]);
  const auto &orLink0 = builder.addCell(OR, inputs[0], ~inputs[0]);
  const auto &orLink1 = builder.addCell(OR, inputs[1], inputs[2]);
  builder.addOutput(andLink0);
  builder.addOutput(orLink0);
  builder.addOutput(orLink1);
  const auto &subnet = Subnet::get(builder.make());

  eda::gate::solver::Solver solver;
  debugger::Verifier verifier(subnet, solver);

  auto prop1 = verifier.makeEqualty(subnet.getOut(0), 1);
  auto prop3 = verifier.makeEqualty(subnet.getOut(0), 0);
  auto prop2 = verifier.makeEqualty(subnet.getOut(1), 1);
  auto prop4 = verifier.makeEqualty(subnet.getOut(1), 0);
  auto prop5 = verifier.makeEqualty(subnet.getOut(2), 0);
  auto prop6 = verifier.makeEqualty(subnet.getOut(2), 1);

  EXPECT_FALSE(verifier.checkAlways(verifier.getProperty(prop1)));
  EXPECT_TRUE(verifier.checkAlways(verifier.getProperty(prop2)));
  EXPECT_TRUE(verifier.checkAlways(verifier.getProperty(prop3)));
  EXPECT_FALSE(verifier.checkAlways(verifier.getProperty(prop4)));
  EXPECT_FALSE(verifier.checkAlways(verifier.getProperty(prop5)));
  EXPECT_FALSE(verifier.checkAlways(verifier.getProperty(prop6)));
}

TEST(VerifierTest, InnerLinkCheck) {
  SubnetBuilder builder;
  const auto &inputs =  builder.addInputs(2);
  const auto &zero = builder.addCell(ZERO);
  const auto &andLink0 = builder.addCell(AND, inputs[0], zero);
  const auto &andLink1 = builder.addCell(AND, zero, inputs[1]);
  const auto &andLink2 = builder.addCell(AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &subnet = Subnet::get(builder.make());

  eda::gate::solver::Solver solver;
  debugger::Verifier verifier(subnet, solver);

  Subnet::Link lhs, rhs;
  const auto entries = subnet.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i == 5) {
      lhs = entries[i].cell.link[0];
      rhs = entries[i].cell.link[1];
    }
  }

  auto prop1 = verifier.makeEqualty(lhs, rhs);

  EXPECT_TRUE(verifier.checkEventually(verifier.getProperty(prop1)));
}

} // namespace eda::gate::model
