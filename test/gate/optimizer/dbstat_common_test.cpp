//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/model/printer/net_printer.h"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/get_dbstat.h"
#include "gate/optimizer/npnstatdb.h"

#include "kitty/kitty.hpp"
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

// compare 2 files and output the error line when finds
static bool compareFiles(const std::string &p1, const std::string &p2) {
  std::ifstream f1(p1, std::ios::binary);
  std::ifstream f2(p2, std::ios::binary);

  if(!f1.is_open() || !f2.is_open()){
    return false;
  }

  std::string line;
  std::string line2;

  while (std::getline(f1, line) && std::getline(f2, line2)) {
    if (line.compare(line2) != 0) {
      std::cout << "Error line:" << std::endl;
      std::cout << line << " | " << line2 << std::endl;
      return false;
    }
  }
  return true;
}

static void deleteFileIfExists(const std::string &filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

// creation fo NpnDatabase
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T> 
static void npndbCreate(T npndb) {
  static SubnetID id1 = makeSubnet3AndOrXor();
  static SubnetID id2 = makeSubnet2AndOr();
  static SubnetID id3 = makeSubnetXorOrXor();
  static SubnetID id4 = makeSubnetAndOrXor();  

  npndb->push(id1);
  npndb->push(id2);
  npndb->push(id3);
  npndb->push(id4);
}

// print Info of Subnet in file
static void printInfo(const std::string &filename, SubnetID id) {
  std::ofstream out(filename);
  if (out.is_open()) {
    NpnDatabase::printInfoSub(out, Subnet::get(id));
  out.close();
  }
}

// print DOT file of Subnet
static void printDot(const std::string &filename, SubnetID id,
                       const std::string name = "") {
  std::ofstream out(filename);
  if (out.is_open()) {
    eda::gate::model::print(out, eda::gate::model::DOT, name, Subnet::get(id));
    out.close();
  }
}

// print Info of NpnDatabase Subnet in file
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T>
static void printNpnInfo(const std::string &filename, const T npndb, 
                         const TT tt) {
  std::ofstream out(filename);
  if (out.is_open()) {
    npndb->printInfo(out, tt);
  out.close();
  }
}

// print DOT file of NpnDatabase Subnet
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T>
static void printNpnDot(const std::string &filename, const T npndb, 
                         const TT tt, const std::string name = "") {
  std::ofstream out(filename);
  if (out.is_open()) {
    npndb->printDot(out, tt, name);
  out.close();
  }
}

// print DOT file of NpnDatabase Subnet
// T accepts variable types NpnDatabase and NpnStatDatabase
template <typename T>
static void printNpnDotFile(const std::string &filename, const T npndb, 
                         const TT tt, const std::string name = "") {
  npndb->printDotFile(tt, filename, name);
}
