//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/dbstat_common_test.cpp"
#include "gate/optimizer/get_dbstat.h"
#include "gate/optimizer/npndb.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::utils;

using TT = kitty::dynamic_truth_table;

// print result of getDbStat in file
static void printGetDbStat(const std::string &filename, NpnDbConfig conf) {
  std::ofstream out(filename);
  if (out.is_open()) {
    getDbStat(out, conf);
    out.close();
  }
}

// print string in file
static void printMsg(const std::string &filename, const std::string msg = "") {
  std::ofstream out(filename);
  if (out.is_open()) {
    out << msg;
    out.close();
  }
}

// print Info and DOT module in file
static void printInfoDot(const std::string &filename, SubnetID id,
                         const std::string name = "") {
  std::ofstream out(filename);
  if (out.is_open()) {
    NpnDatabase::printInfoSub(out, Subnet::get(id));
    ModelPrinter::getPrinter(ModelPrinter::Format::DOT)
        .print(out, Subnet::get(id), name);
    out.close();
  }
}

// test of incorrect config and compare error messages
static void errorTest(const std::string f1, const NpnDbConfig conf,
                      const std::string f2, const std::string msg) {
  printGetDbStat(f1, conf);
  printMsg(f2, msg);
  ASSERT_TRUE(compareFiles(f1, f2));
}

// test of messages of critical errors
TEST(GetStatTest, GetStatTestPrintErrors) {

  // DB creation
  std::string fnDb = "db.rwdb";
  std::string fnBackup = "backup.txt";
  std::string fnBackupCorrect = "backupCorrect.txt";

  NpnDatabase npndb;
  npndbCreate(&npndb);
  npndb.exportTo(fnDb);

  NpnDbConfig conf;
  conf.ttSize = 2;
  conf.outType = OutType::BOTH;
  conf.outName = "";
  conf.binLines.clear();
  conf.binLines.push_back("1000");

  // test 1. Incorrect dbPath
  conf.dbPath = "error_string";

  errorTest(fnBackup, conf, fnBackupCorrect, "Wrong DB path\n");

  // test 2. Wrong type of the file
  printMsg("test.txt", "some info\n");
  conf.dbPath = "test.txt";
  errorTest(fnBackup, conf, fnBackupCorrect, "Wrong format of DB\n");
  deleteFileIfExists("test.txt");

  // test 3. Wrong ttsize
  conf.dbPath = fnDb;
  conf.ttSize = 0;
  errorTest(fnBackup, conf, fnBackupCorrect,
            "An incalculable size: 0\nSize of inputs should be from 1 to 6\n");

  // test 4. Wrong ttsize
  conf.dbPath = fnDb;
  conf.ttSize = 100;
  errorTest(
      fnBackup, conf, fnBackupCorrect,
      "An incalculable size: 100\nSize of inputs should be from 1 to 6\n");

  // test 5. Empty binLines
  conf.ttSize = 2;
  conf.binLines.clear();
  errorTest(fnBackup, conf, fnBackupCorrect,
            "Empty binary lines!\n");

  // test 6. bad-formatted binLines
  conf.binLines.clear();
  conf.binLines.push_back("erro");
  errorTest(fnBackup, conf, fnBackupCorrect, "The line should be binary\n");

  // test 7. wrong length binLines
  conf.binLines.clear();
  conf.binLines.push_back("10000");
  errorTest(
      fnBackup, conf, fnBackupCorrect,
      "Wrong length of the values, your line size is 5 correct size is 4\n");

  // test 8. no suitable Subnets
  conf.binLines.clear();
  conf.binLines.push_back("1001");
  errorTest(fnBackup, conf, fnBackupCorrect,
            "No equivalent scheme has been found\n");

  deleteFileIfExists(fnDb);
  deleteFileIfExists(fnBackup);
  deleteFileIfExists(fnBackupCorrect);
}

// test of correct config with all possible types and compare outputs
static void correctTestTypes(const std::string f1, NpnDbConfig conf,
                             const std::string f2, const SubnetID id,
                             const std::string msg) {

  std::string fl = "";
  if (conf.outName.size() < 4 ||
      conf.outName.substr(conf.outName.size() - 4) != ".dot") {
    fl = ".dot";
  }

  conf.outType = OutType::DOT;
  printGetDbStat(f1, conf);
  printDot(f2, id, msg);

  if (conf.outName == "") {
    ASSERT_TRUE(compareFiles(f1, f2));
  } else {
    ASSERT_TRUE(compareFiles(conf.outName + fl, f2));
  }

  conf.outType = OutType::INFO;
  printGetDbStat(f1, conf);
  printInfo(f2, id);
  ASSERT_TRUE(compareFiles(f1, f2));

  conf.outType = OutType::BOTH;
  printGetDbStat(f1, conf);

  if (conf.outName == "") {
    printInfoDot(f2, id, msg);
    ASSERT_TRUE(compareFiles(f1, f2));
  } else {
    printDot(f2, id, msg);
    ASSERT_TRUE(compareFiles(conf.outName + fl, f2));
    printInfo(f2, id);
    ASSERT_TRUE(compareFiles(f1, f2));
  }
}

// creates config from binLines with all possible types of file name and compare outputs
static void correctTest(const size_t size, const SubnetID id,
                        std::vector<std::string> binLinesInput) {

  // DB creation
  std::string fnDb = "db.rwdb";
  std::string fnBackup = "backup.txt";
  std::string fnBackupCorrect = "backupCorrect.txt";

  NpnDatabase npndb;
  npndbCreate(&npndb);

  npndb.exportTo(fnDb);

  NpnDbConfig conf;

  conf.dbPath = fnDb;
  conf.ttSize = size;
  conf.outName = "";
  conf.binLines.clear();
  for (const auto &binLine : binLinesInput) {
    conf.binLines.push_back(binLine);
  }
  correctTestTypes(fnBackup, conf, fnBackupCorrect, id, binLinesInput[0]);

  conf.outName = "out.dot";
  correctTestTypes(fnBackup, conf, fnBackupCorrect, id, binLinesInput[0]);

  conf.outName = "out";
  correctTestTypes(fnBackup, conf, fnBackupCorrect, id, binLinesInput[0]);

  deleteFileIfExists(fnDb);
  deleteFileIfExists(fnBackup);
  deleteFileIfExists(fnBackupCorrect);
  deleteFileIfExists("out.dot");
}

// test of correct working with single bin line
TEST(GetStatTest, GetStatTestPrintCorrectSingleLine) {
  std::vector<std::string> binLinesInput;
  binLinesInput.clear();
  binLinesInput.push_back("1111111110000000");

  correctTest(4, makeSubnet2AndOr(), binLinesInput);
}

// test of correct working with multiple bin line
TEST(GetStatTest, GetStatTestPrintCorrectMultipleLine) {
  std::vector<std::string> binLinesInput;
  binLinesInput.clear();
  binLinesInput.push_back("0110");
  binLinesInput.push_back("1000");
  binLinesInput.push_back("1110");

  correctTest(2, makeSubnetAndOrXor(), binLinesInput);
}