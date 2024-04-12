//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/techmapper/library/cell.h"
#include "util/logging.h"

#include "gtest/gtest.h"

#include <vector>

using namespace eda::gate::techmapper;
using namespace eda::gate::model;

std::string techLibPath =
  std::string(getenv("UTOPIA_HOME")) + "/test/data/gate/techmapper";

bool checkLibParser(std::string libertyPath) {
  //const std::string pathToLiberty = libertyPath + "/liberty";
  std::vector<eda::gate::model::CellTypeID> combCells;
  std::vector<eda::gate::model::CellTypeID> cellsFF;
  std::vector<eda::gate::model::CellTypeID> cellsFFrs;
  std::vector<eda::gate::model::CellTypeID> cellsLatch;
  LibraryCells::readLibertyFile(libertyPath, combCells, cellsFF, cellsFFrs, cellsLatch);

//#ifdef UTOPIA_DEBUG
  for(const auto& cell : combCells) {
    std::cout << CellType::get(cell).getName() << std::endl;
  }
//#endif
  return true;
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(techLibPath + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(techLibPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
} // namespace eda::gate::techmapper
