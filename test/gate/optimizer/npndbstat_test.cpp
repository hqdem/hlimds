//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/dbstat_common_test.cpp"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/npnstatdb.h"

#include "kitty/kitty.hpp"
#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::utils;

using TT = kitty::dynamic_truth_table;

// test of the function printDot, printDotFile
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T> void testDOT(T npndb, TT tt, SubnetID id) {
  std::string filename1 = "test1.dot";
  std::string filename2 = "test2.dot";
  std::string filename3 = "test3.dot";

  printDot(filename1, id, "test");
  printNpnDot(filename2, &npndb, tt, "test");
  printNpnDotFile(filename3, &npndb, tt, "test");

  ASSERT_TRUE(compareFiles(filename1, filename2));
  ASSERT_TRUE(compareFiles(filename2, filename3));

  deleteFileIfExists(filename1);
  deleteFileIfExists(filename2);
  deleteFileIfExists(filename3);
}

// test of the function printInfo
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T> void testInfo(T npndb, TT tt, SubnetID id) {
  std::string filename1 = "test1.txt";
  std::string filename2 = "test2.txt";

  printInfo(filename1, id);
  printNpnInfo(filename2, &npndb, tt);

  ASSERT_TRUE(compareFiles(filename1, filename2));

  deleteFileIfExists(filename1);
  deleteFileIfExists(filename2);
}

// test output number 1, check makeSubnet3AndOrXor() Subnet
template <typename T> static void testNpn1(T npndb) {
  npndbCreate(&npndb);
  // equal to makeSubnet3AndOrXor()
  TT tt(5);
  kitty::create_from_chain(
      tt, {"x6 = x1 & x2", "x7 = x3 & x6", "x8 = x4 ^ x5", "x9 = x7 | x8"});
  testDOT(npndb, tt, makeSubnet3AndOrXor());
  testInfo(npndb, tt, makeSubnet3AndOrXor());
}

// test for NpnDatabase
TEST(NpnDatabaseStatTest, NpnDatabasePrintDOT1) {
  NpnDatabase npndb;
  testNpn1(npndb);
}

// test for NpnStatDatabase
TEST(NpnStatDatabaseStatTest, NpnStatDatabasePrintDOT1) {
  NpnStatDatabase npndb;
  testNpn1(npndb);
}

// test output number 2, check makeSubnetXorOrXor() Subnet
template <typename T> static void testNpn2(T npndb) {
  npndbCreate(&npndb);
  // equal to makeSubnetXorOrXor()
  TT tt(3);
  kitty::create_from_chain(tt,
                           {"x4 = x1 ^ x2", "x5 = x2 ^ x3", "x6 = x4 | x5"});

  testDOT(npndb, tt, makeSubnetXorOrXor());
  testInfo(npndb, tt, makeSubnetXorOrXor());
}

// test for NpnDatabase
TEST(NpnDatabaseStatTest, NpnDatabasePrintDOT2) {
  NpnDatabase npndb;
  testNpn2(npndb);
}

// test for NpnStatDatabase
TEST(NpnStatDatabaseStatTest, NpnStatDatabasePrintDOT2) {
  NpnStatDatabase npndb;
  testNpn2(npndb);
}

// test output number 3, check makeSubnetAndOrXor() Subnet
template <typename T> static void testNpn3(T npndb) {
  npndbCreate(&npndb);
  // equal to makeSubnetAndOrXor()
  TT tt(2);
  kitty::create_from_chain(tt, {"x3 = x1 & x2"});

  testDOT(npndb, tt, makeSubnetAndOrXor());
  testInfo(npndb, tt, makeSubnetAndOrXor());
}

// test for NpnDatabase
TEST(NpnDatabaseStatTest, NpnDatabasePrintDOT3) {
  NpnDatabase npndb;
  testNpn3(npndb);
}

// test for NpnStatDatabase
TEST(NpnStatDatabaseStatTest, NpnStatDatabasePrintDOT3) {
  NpnStatDatabase npndb;
  testNpn3(npndb);
}