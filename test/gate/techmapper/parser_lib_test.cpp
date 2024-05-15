//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/library/cell.h"
#include "gate/techmapper/library/liberty_manager.h"
#include "test_util.h"
#include "util/logging.h"

#include "gtest/gtest.h"

#include <vector>

using namespace eda::gate::techmapper;
using namespace eda::gate::model;

const std::filesystem::path techLibPath =
    getHomePath() / "test/data/gate/techmapper";

bool checkLibParser(std::string libertyPath) {
  LibraryManager::get().loadLibrary(libertyPath);
  std::cout << "Loaded Liberty: " << libertyPath << std::endl;
  std::vector<eda::gate::model::CellTypeID> combCells;
  std::vector<eda::gate::model::CellTypeID> cellsFF;
  std::vector<eda::gate::model::CellTypeID> cellsFFrs;
  std::vector<eda::gate::model::CellTypeID> cellsLatch;
  LibraryCells::readLibertyFile(combCells, cellsFF, cellsFFrs, cellsLatch);

#ifdef UTOPIA_DEBUG
  for(const auto& cell : combCells) {
    std::cout << CellType::get(cell).getName() << std::endl;
  }
#endif // UTOPIA_DEBUG
  return true;
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  checkLibParser(techLibPath + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  checkLibParser(techLibPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
}

} // namespace eda::gate::techmapper
