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

const static std::string homePath = eda::env::getHomePathAsString();

const static std::string binPath = homePath +
    "/build/src/umain";
const static std::string gatePath = homePath +
    "/test/data/gate";
const static std::string dataPath = gatePath +
    "/interface";
const static std::string graphMl = homePath +
    "/test/data/openabcd-subset/graphml/sasc_orig.bench.graphml";
const static std::string output = homePath +
    "/output/test/shell/";
const static std::string liberty = gatePath +
    "/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

static inline bool check(std::string command) {
  return system(command.c_str()) == 0;
}

static void test(std::string script, bool expect) {
  std::stringstream command;
  command << binPath << " -e "
          << "\""
          << script << ";"
          << "exit"
          << "\""
          << " > /dev/null 2>/dev/null";

  if (expect) {
    EXPECT_TRUE(check(command.str()));
  } else {
    EXPECT_FALSE(check(command.str()));
  }
}

TEST(UtopiaShell, ReadGraphMlAlreadyLoaded) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "read_graphml " << graphMl;
  test(script.str(), false);
}

TEST(UtopiaShell, ReadGraphMlNoFile) {
  test("read_graphml", false);
}

TEST(UtopiaShell, ReadGraphMlFileNotExist) {
  test("read_graphml this_file_does_not_exist", false);
}

TEST(UtopiaShell, ReadGraphMl) {
  std::stringstream script;
  script << "read_graphml " << graphMl;
  test(script.str(), true);
}

TEST(UtopiaShell, DeleteDesignNotLoaded) {
  test("delete_design", false);
}

TEST(UtopiaShell, DeleteDesign) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "delete_design";
  test(script.str(), true);
}

TEST(UtopiaShell, Help) {
  test("help", true);
}

TEST(UtopiaShell, HelpCommandNotExist) {
  test("help this_command_does_not_exist", false);
}

TEST(UtopiaShell, HelpCommand) {
  test("help logopt", true);
}

TEST(UtopiaShell, LecNoDesign) {
  test("lec", false);
}

TEST(UtopiaShell, LecNoPoints) {
  std::stringstream script;
  script << "read_graphml " << graphMl + ";"
         << "lec";
  test(script.str(), false);
}

static void testLec(const std::string &method) {
  std::stringstream script;
  script << "read_graphml " << graphMl + ";"
         << "save_point p1;"
         << "logopt rw;"
         << "save_point p2;"
         << "lec --method " << method << " p1 p2";
  test(script.str(), true);
}

TEST(UtopiaShell, LecBdd) {
  testLec("bdd");
}

TEST(UtopiaShell, LecFra) {
  testLec("fra");
}

TEST(UtopiaShell, DISABLED_LecRnd) {
  testLec("rnd");
}

TEST(UtopiaShell, LecSat) {
  testLec("sat");
}

TEST(UtopiaShell, ReadLibertyNoFile) {
  test("read_liberty", false);
}

TEST(UtopiaShell, ReadLibertyFileNotExist) {
  test("read_liberty this_file_does_not_exist", false);
}

TEST(UtopiaShell, ReadLiberty) {
  std::stringstream script;
  script << "read_liberty " << liberty;
  test(script.str(), true);
}

TEST(UtopiaShell, StatDesignNoDesign) {
  test("stat_design", false);
}

TEST(UtopiaShell, StatDesign) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "stat_design";
  test(script.str(), true);
}

#if 0
TEST(UtopiaShell, FirNoFiles) {
  test("verilog_to_fir", false);
}

TEST(UtopiaShell, FirNotExists) {
  test("verilog_to_fir this_file_does_not_exist", false);
}
#endif

static void testWriteNoDesign(const std::string &format) {
  std::stringstream script;
  script << "write_" << format;
  test(script.str(), false);
}

static void testWriteNoFile(const std::string &format) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "write_" << format;
  test(script.str(), false);
}

static void testWriteDesign(const std::string &format,
                            const std::string &file) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "write_" << format << " "
         << output << file;
  test(script.str(), true);
}

static void testWriteNoSubnet(const std::string &format,
                              const std::string &file) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "write_" << format << " --subnet-index 2 "
         << output << file;
  test(script.str(), false);
}

static void testWriteSubnet(const std::string &format,
                            const std::string &file) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "write_" << format << " --subnet-index 0 "
         << output << file;
  test(script.str(), true);
}

static void testWriteSubnets(const std::string &format,
                             const std::string &file) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "write_" << format << " --subnet-split "
         << output << file;
  test(script.str(), true);
}

#define TEST_WRITE_FORMAT(format, file)\
TEST(UtopiaShell, WriteNoDesign_##format) {\
  testWriteNoDesign(#format);\
}\
TEST(UtopiaShell, WriteNoFile_##format) {\
  testWriteNoFile(#format);\
}\
TEST(UtopiaShell, WriteDesign_##format) {\
  testWriteDesign(#format, file);\
}\
TEST(UtopiaShell, WriteNoSubnet_##format) {\
  testWriteNoSubnet(#format, file);\
}\
TEST(UtopiaShell, WriteSubnet_##format) {\
  testWriteSubnet(#format, file);\
  testWriteSubnets(#format, file);\
}

TEST_WRITE_FORMAT(debug, "design.out")
TEST_WRITE_FORMAT(dot, "design.dot")
TEST_WRITE_FORMAT(verilog, "design.v")

TEST(UtopiaShell, LogOptNoDesign) {
  test("logopt", false);
}

static void testLogOpt(const std::string &pass) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "logopt " << pass;
  test(script.str(), true);
}

TEST(UtopiaShell, LogOptAig) {
  testLogOpt("aig");
}

#if 0
TEST(UtopiaShell, LogOptMig) {
  testLogOpt("mig");
}
#endif

TEST(UtopiaShell, LogOptB) {
  testLogOpt("b");
}

TEST(UtopiaShell, LogOptRw) {
  testLogOpt("rw");
}

TEST(UtopiaShell, LogOptRwK) {
  testLogOpt("rw -k 3");
}

TEST(UtopiaShell, LogOptRwz) {
  testLogOpt("rwz");
}

TEST(UtopiaShell, LogOptRf) {
  testLogOpt("rf");
}

TEST(UtopiaShell, LogOptRfz) {
  testLogOpt("rfz");
}

TEST(UtopiaShell, LogOptRfa) {
  testLogOpt("rfa");
}

TEST(UtopiaShell, LogOptRfd) {
  testLogOpt("rfd");
}

TEST(UtopiaShell, LogOptRfp) {
  testLogOpt("rfp");
}

TEST(UtopiaShell, LogOptRs) {
  testLogOpt("rs");
}

TEST(UtopiaShell, LogOptRsK) {
  testLogOpt("rs -k 6");
}

TEST(UtopiaShell, LogOptRsKN) {
  testLogOpt("rs -k 6 -n 12");
}

TEST(UtopiaShell, LogOptRsz) {
  testLogOpt("rsz");
}

TEST(UtopiaShell, LogOptRszK) {
  testLogOpt("rsz -k 6");
}

TEST(UtopiaShell, LogOptRszKN) {
  testLogOpt("rsz -k 6 -n 12");
}

TEST(UtopiaShell, LogOptResyn) {
  testLogOpt("resyn");
}

TEST(UtopiaShell, LogOptResyn2) {
  testLogOpt("resyn2");
}

TEST(UtopiaShell, LogOptResyn2a) {
  testLogOpt("resyn2a");
}

TEST(UtopiaShell, DISABLED_LogOptResyn3) {
  testLogOpt("resyn3");
}

TEST(UtopiaShell, LogOptCompress) {
  testLogOpt("compress");
}

TEST(UtopiaShell, LogOptCompress2) {
  testLogOpt("compress2");
}

TEST(UtopiaShell, TechMapNoDesign) {
  test("techmap", false);
}

TEST(UtopiaShell, TechMapNoLibrary) {
  std::stringstream script;
  script << "read_graphml " << graphMl + ";"
         << "techmap";
  test(script.str(), false);
}

static void testTechMap(const std::string &criterion) {
  std::stringstream script;
  script << "read_graphml " << graphMl << ";"
         << "read_liberty " << liberty << ";"
         << "techmap --objective " << criterion;
  test(script.str(), true);
}

TEST(UtopiaShell, TechMapArea) {
  testTechMap("area");
}

TEST(UtopiaShell, TechMapDelay) {
  testTechMap("delay");
}

TEST(UtopiaShell, TechMapPower) {
  testTechMap("power");
}
