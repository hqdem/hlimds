//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "kitty/kitty.hpp"

#include "gate/techoptimizer/library/cell.h"
#include "gate/model/gnet_test.h"
#include "gate/optimizer/rwdatabase.h"
#include "util/logging.h"

#include "gtest/gtest.h"
#include <filesystem>
#include <vector>

using namespace eda::gate::optimizer;
using namespace eda::gate::tech_optimizer;
using namespace eda::gate::model;

//const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
//const std::filesystem::path libertyDirrect = homePath / "test" / "data" / "gate" / "tech_mapper";
std::string techMapPath = std::string(getenv("UTOPIA_HOME")) + "/test/data/gate/tech_mapper";

bool checkLibParser(std::string libertyPath) {
  //const std::string pathToLiberty = libertyPath + "/liberty";
  std::vector<eda::gate::model::CellTypeID> combCells;
  std::vector<eda::gate::model::CellTypeID> cellsFF;
  std::vector<eda::gate::model::CellTypeID> cellsFFrs;
  std::vector<eda::gate::model::CellTypeID> cellsLatch;
  LibraryCells::readLibertyFile(libertyPath, combCells, cellsFF, cellsFFrs, cellsLatch);

#ifdef UTOPIA_DEBUG
  for(const auto& cell : combCells) {
    std::cout << cell->getName() << std::endl;
  }
#endif

  return true;
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(techMapPath + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(techMapPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
}