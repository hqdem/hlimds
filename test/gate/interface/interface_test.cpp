//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/env.h"

#include "gtest/gtest.h"

#include <sstream>
#include <string>

const std::string homePath = eda::env::getHomePathAsString();
const std::string binPath = homePath + "/build/src/umain";
const std::string gate = homePath + "/test/data/gate";
const std::string dataPath = gate + "/interface";
const std::string sasc_orig = gate +
    "/parser/graphml/OpenABC/graphml_openabcd/sasc_orig.bench.graphml";
const std::string output = homePath + "/output/test/interface/";
const std::string liberty = gate +
    "/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

static bool check(std::string command) {
  bool flag = true;
  const int res = system(command.c_str());
  if (res != 0) {
    flag = false;
  }
  return flag;
}

static void test(std::string script, bool expect) {
  std::string command;
  command = binPath + " -e " + "\"" + script + "\nexit" + "\"" +
      " > /dev/null 2>/dev/null";
  if (expect) {
    EXPECT_TRUE(check(command));
  } else {
    EXPECT_FALSE(check(command));
  }
}

TEST(TclInterfaceTest, writeDesignNotLoaded) {
  test("write_verilog", false);
}

TEST(TclInterfaceTest, writeDesign) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_verilog";
  test(script.str(), false);
}

TEST(TclInterfaceTest, writeSubnetIncorrectNumber) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_verilog --subnet 2 design.v";
  test(script.str(), false);
}

TEST(TclInterfaceTest, writeSubnet) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_verilog --subnet 0 design.v";
  test(script.str(), true);
}

TEST(TclInterfaceTest, readGraphMlAlreadyLoaded) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "read_graphml " << sasc_orig;
  test(script.str(), false);
}

TEST(TclInterfaceTest, readGraphMlNoFile) {
  test("read_graphml", false);
}

TEST(TclInterfaceTest, readGraphMlFileNotExist) {
  test("read_graphml this_file_does_not_exist", false);
}

TEST(TclInterfaceTest, readGraphMl) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig;
  test(script.str(), true);
}

TEST(TclInterfaceTest, deleteDesignNotLoaded) {
  test("delete_design", false);
}

TEST(TclInterfaceTest, deleteDesign) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "delete_design";
  test(script.str(), true);
}

TEST(TclInterfaceTest, help) {
  test("help", true);
}

TEST(TclInterfaceTest, lecNoDesign) {
  test("lec", false);
}

TEST(TclInterfaceTest, lecNoPoints) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "lec";
  test(script.str(), false);
}

TEST(TclInterfaceTest, lecBdd) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "save_point p1;"
         << "logopt rw;"
         << "save_point p2;"
         << "lec --method bdd p1 p2";
  test(script.str(), true);
}

TEST(TclInterfaceTest, lecFra) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "save_point p1;"
         << "logopt rw;"
         << "save_point p2;"
         << "lec --method fra p1 p2";
  test(script.str(), true);
}

TEST(TclInterfaceTest, lecRnd) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "save_point p1;"
         << "logopt rw;"
         << "save_point p2;"
         << "lec --method rnd p1 p2";
  test(script.str(), true);
}

TEST(TclInterfaceTest, lecSat) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "save_point p1;"
         << "logopt rw;"
         << "save_point p2;"
         << "lec --method sat p1 p2";
  test(script.str(), true);
}

TEST(TclInterfaceTest, readLibertyNoFile) {
  test("read_liberty", false);
}

TEST(TclInterfaceTest, readLibertyFileNotExist) {
  test("read_liberty this_file_does_not_exist", false);
}

TEST(TclInterfaceTest, readLiberty) {
  std::stringstream script;
  script << "read_liberty " << liberty;
  test(script.str(), true);
}

TEST(TclInterfaceTest, statDesignNoDesign) {
  test("stat_design", false);
}

TEST(TclInterfaceTest, statDesign) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "stat_design";
  test(script.str(), true);
}

#if 0
TEST(TclInterfaceTest, firNoFiles) {
  test("verilog_to_fir", false);
}

TEST(TclInterfaceTest, firNotExists) {
  test("verilog_to_fir this_file_does_not_exist", false);
}
#endif

TEST(TclInterfaceTest, writeDebugNoDesign) {
  test("write_debug", false);
}

TEST(TclInterfaceTest, writeDebugNoFile) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_debug";
  test(script.str(), false);
}

TEST(TclInterfaceTest, writeDebug) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_debug design.out";
  test(script.str(), true);
}

TEST(TclInterfaceTest, writeDotNoDesign) {
  test("write_dot", false);
}

TEST(TclInterfaceTest, writeDotNoFile) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_dot";
  test(script.str(), false);
}

TEST(TclInterfaceTest, writeDot) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_dot design.dot";
  test(script.str(), true);
}

TEST(TclInterfaceTest, writeVerilogNoDesign) {
  test("write_verilog", false);
}

TEST(TclInterfaceTest, writeVerilogNoFile) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_verilog";
  test(script.str(), false);
}

TEST(TclInterfaceTest, writeVerilog) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "write_verilog design.v";
  test(script.str(), true);
}

TEST(TclInterfaceTest, logOptNoDesign) {
  test("logopt", false);
}

static void testLogOpt(const std::string &pass) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "logopt " << pass;
  test(script.str(), true);
}

TEST(TclInterfaceTest, logOptAig) {
  testLogOpt("aig");
}

#if 0
TEST(TclInterfaceTest, logOptMig) {
  testLogOpt("mig");
}
#endif

TEST(TclInterfaceTest, logOptB) {
  testLogOpt("b");
}

TEST(TclInterfaceTest, logOptRw) {
  testLogOpt("rw");
}

TEST(TclInterfaceTest, logOptRwK) {
  testLogOpt("rw -k 3");
}

TEST(TclInterfaceTest, logOptRwz) {
  testLogOpt("rwz");
}

TEST(TclInterfaceTest, logOptRf) {
  testLogOpt("rf");
}

TEST(TclInterfaceTest, logOptRfz) {
  testLogOpt("rfz");
}

TEST(TclInterfaceTest, logOptRfa) {
  testLogOpt("rfa");
}

TEST(TclInterfaceTest, logOptRfd) {
  testLogOpt("rfd");
}

TEST(TclInterfaceTest, logOptRfp) {
  testLogOpt("rfp");
}

TEST(TclInterfaceTest, logOptRs) {
  testLogOpt("rs");
}

TEST(TclInterfaceTest, logOptRsK) {
  testLogOpt("rs -k 6");
}

TEST(TclInterfaceTest, logOptRsKN) {
  testLogOpt("rs -k 6 -n 12");
}

TEST(TclInterfaceTest, logOptRsz) {
  testLogOpt("rsz");
}

TEST(TclInterfaceTest, logOptRszK) {
  testLogOpt("rsz -k 6");
}

TEST(TclInterfaceTest, logOptRszKN) {
  testLogOpt("rsz -k 6 -n 12");
}

TEST(TclInterfaceTest, logOptResyn) {
  testLogOpt("resyn");
}

TEST(TclInterfaceTest, logOptResyn2) {
  testLogOpt("resyn2");
}

TEST(TclInterfaceTest, logOptResyn2a) {
  testLogOpt("resyn2a");
}

#if 0
TEST(TclInterfaceTest, logOptResyn3) {
  testLogOpt("resyn3");
}
#endif

TEST(TclInterfaceTest, logOptCompress) {
  testLogOpt("compress");
}

TEST(TclInterfaceTest, logOptCompress2) {
  testLogOpt("compress2");
}

TEST(TclInterfaceTest, techmapNoDesign) {
  test("techmap", false);
}

TEST(TclInterfaceTest, techmapNoLibrary) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig + ";"
         << "techmap";
  test(script.str(), false);
}

static void testTechMap(const std::string &criterion) {
  std::stringstream script;
  script << "read_graphml " << sasc_orig << ";"
         << "read_liberty " << liberty << ";"
         << "techmap -t " << criterion;
  test(script.str(), true);
}

TEST(TclInterfaceTest, techmapArea) {
  testTechMap("area");
}

TEST(TclInterfaceTest, techmapDelay) {
  testTechMap("delay");
}

TEST(TclInterfaceTest, techmapPower) {
  testTechMap("power");
}
