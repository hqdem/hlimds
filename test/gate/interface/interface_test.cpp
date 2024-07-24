//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/env.h"

#include "gtest/gtest.h"

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

bool check(std::string command) {
  bool flag = true;
  const int res = system(command.c_str());
  if (res != 0) {
    flag = false;
  }
  return flag;
}

void test(std::string script, bool expect) {
  std::string command;
  command = binPath + " -e " + "\"" + script + "\nexit" + "\"" +
      " > /dev/null 2>/dev/null";
  if (expect) {
    EXPECT_TRUE(check(command));
  } else {
    EXPECT_FALSE(check(command));
  }
}

TEST(TclInterfaceTest, writeSubnetNotLoaded) {
  test("write_subnet", false);
}

TEST(TclInterfaceTest, writeSubnetNoNumber) {
  std::string com = "read_graphml " + sasc_orig + "\nwrite_subnet -i 2";
  test(com, false);
}

TEST(TclInterfaceTest, writeSubnet) {
  std::string com = "read_graphml " + sasc_orig + "\nwrite_subnet";
  test(com, true);
}

TEST(TclInterfaceTest, readGraphmlAlreadyUploaded) {
  std::string com = "read_graphml " + sasc_orig +
      "\nread_graphml " + sasc_orig;
  test(com, false);
}

TEST(TclInterfaceTest, readGraphmlNotSpecifiedFile) {
  test("read_graphml", false);
}

TEST(TclInterfaceTest, readGraphmlNotExists) {
  test("read_graphml notExists", false);
}

TEST(TclInterfaceTest, readGraphml) {
  std::string com = "read_graphml " + sasc_orig;
  test(com, true);
}

TEST(TclInterfaceTest, clear) {
  test("clear", true);
}

TEST(TclInterfaceTest, help) {
  test("help", true);
}

TEST(TclInterfaceTest, lecNoDesign) {
  std::string com = "lec";
  test(com, false);
}

TEST(TclInterfaceTest, lecNothingToCompareWith) {
  std::string com = "read_graphml " + sasc_orig + "\nlec";
  test(com, false);
}

TEST(TclInterfaceTest, lecBdd) {
  std::string com = "read_graphml " + sasc_orig + "\npass rw\nlec -m bdd";
  test(com, true);
}

TEST(TclInterfaceTest, lecFra) {
  std::string com = "read_graphml " + sasc_orig + "\npass rw\nlec -m fra";
  test(com, true);
}

TEST(TclInterfaceTest, lecRnd) {
  std::string com = "read_graphml " + sasc_orig + "\npass rw\nlec -m rnd";
  test(com, true);
}

TEST(TclInterfaceTest, lecSat) {
  std::string com = "read_graphml " + sasc_orig + "\npass rw\nlec -m sat";
  test(com, true);
}

TEST(TclInterfaceTest, readLiberyNoPathTo) {
  test("read_liberty", false);
}

TEST(TclInterfaceTest, readLiberyFileNotExists) {
  test("read_liberty notExists", false);
}

TEST(TclInterfaceTest, readLibery) {
  std::string com = "read_liberty " + liberty;
  test(com, true);
}

TEST(TclInterfaceTest, statsNoDesign) {
  test("stats", false);
}

TEST(TclInterfaceTest, statsPhysicalNotAvalible) {
  std::string com = "read_graphml " + sasc_orig + "\nstats";
  test(com, false);
}

TEST(TclInterfaceTest, stats) {
  std::string com = "read_graphml " + sasc_orig + "\nstat -l";
  test(com, true);
}

TEST(TclInterfaceTest, firNoFiles) {
  test("verilog_to_fir", false);
}

TEST(TclInterfaceTest, firNotExists) {
  test("verilog_to_fir notExists", false);
}

TEST(TclInterfaceTest, writeDesignNoDesign) {
  test("write_design", false);
}

TEST(TclInterfaceTest, writeDesignNonExistentFormat) {
  std::string com = "read_graphml " + sasc_orig + "\nwrite_design -f notExist";
  test(com, false);
}

TEST(TclInterfaceTest, writeDesignVerilog) {
  std::string com = "read_graphml " + sasc_orig +
      "\nwrite_design -f verilog -p " + output + "design.v";
  test(com, true);
}

TEST(TclInterfaceTest, writeDesignSimple) {
  std::string com = "read_graphml " + sasc_orig +
      "\nwrite_design -f simple -p " + output + "design.s";
  test(com, true);
}

TEST(TclInterfaceTest, writeDesignDot) {
  std::string com = "read_graphml " + sasc_orig +
      "\nwrite_design -f dot -p " + output + "design.dot";
  test(com, true);
}

TEST(TclInterfaceTest, passNoDesign) {
  test("pass", false);
}

TEST(TclInterfaceTest, passAig) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass aig";
  test(com, true);
}

TEST(TclInterfaceTest, passMig) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass aig";
  test(com, true);
}

TEST(TclInterfaceTest, passB) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass b";
  test(com, true);
}

TEST(TclInterfaceTest, passRw) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rw";
  test(com, true);
}

TEST(TclInterfaceTest, passRwK) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rw -k 3";
  test(com, true);
}

TEST(TclInterfaceTest, passRwZ) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rwz";
  test(com, true);
}

TEST(TclInterfaceTest, passRf) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rf";
  test(com, true);
}

TEST(TclInterfaceTest, passRfz) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rfz";
  test(com, true);
}

TEST(TclInterfaceTest, passRfa) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rfa";
  test(com, true);
}

TEST(TclInterfaceTest, passRfd) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rfd";
  test(com, true);
}

TEST(TclInterfaceTest, passRfp) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rfp";
  test(com, true);
}

TEST(TclInterfaceTest, passRs) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rs";
  test(com, true);
}

TEST(TclInterfaceTest, passRsk) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rs -k 6";
  test(com, true);
}

TEST(TclInterfaceTest, passRsKN) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass rs -k 6 -n 12";
  test(com, true);
}

// FIXME no implementation for these passes
// TEST(TclInterfaceTest, passRsz) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass rsz";
//   test(com, true);
// }

// TEST(TclInterfaceTest, passRszK) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass rsz -k 6";
//   test(com, true);
// }

// TEST(TclInterfaceTest, passRszKN) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass rsz -k 6 -n 12";
//   test(com, true);
// }

// TEST(TclInterfaceTest, ma) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass ma";
//   test(com, true);
// }

// TEST(TclInterfaceTest, md) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass md";
//   test(com, true);
// }

// TEST(TclInterfaceTest, mp) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass mp";
//   test(com, true);
// }

TEST(TclInterfaceTest, passResyn) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass resyn";
  test(com, true);
}

TEST(TclInterfaceTest, passResyn2) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass resyn2";
  test(com, true);
}

TEST(TclInterfaceTest, passResyn2a) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass resyn2a";
  test(com, true);
}

// FIXME
// TEST(TclInterfaceTest, passResyn3) {
//   std::string com = "read_graphml " + sasc_orig +
//       "\npass resyn3";
//   test(com, true);
// }

TEST(TclInterfaceTest, compress) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass compress";
  test(com, true);
}

TEST(TclInterfaceTest, compress2) {
  std::string com = "read_graphml " + sasc_orig +
      "\npass compress2";
  test(com, true);
}

TEST(TclInterfaceTest, techmapNoDesign) {
  test("techmap", false);
}

TEST(TclInterfaceTest, techmapNoPathLiberty) {
  std::string com = "read_graphml " + sasc_orig + "\n techmap";
  test(com, false);
}

TEST(TclInterfaceTest, techmapAf) {
  std::string com = "read_graphml " + sasc_orig +
      "\nread_liberty " + liberty + "\n techmap -t af";
  test(com, true);
}

TEST(TclInterfaceTest, techmapArea) {
  std::string com = "read_graphml " + sasc_orig +
      "\nread_liberty " + liberty + "\n techmap -t area";
  test(com, true);
}

TEST(TclInterfaceTest, techmapDelay) {
  std::string com = "read_graphml " + sasc_orig +
      "\nread_liberty " + liberty + "\n techmap -t delay";
  test(com, true);
}

TEST(TclInterfaceTest, techmapPower) {
  std::string com = "read_graphml " + sasc_orig +
      "\nread_liberty " + liberty + "\n techmap -t power";
  test(com, true);
}
