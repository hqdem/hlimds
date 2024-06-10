//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/liberty_manager.h"
#include "gate/library/library.h"
#include "test_util.h"
#include "util/logging.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <vector>

using path = std::filesystem::path;

const path home = eda::env::getHomePath();
const path techLibPath = home /
                     "test/data/gate/techmapper";

namespace eda::gate::library {
bool checkLibParser(std::string libertyPath) {
  LibertyManager::get().loadLibrary(libertyPath);
  std::cout << "Loaded Liberty: " << libertyPath << std::endl;

  SCLibrary standardCells; // TODO

#ifdef UTOPIA_DEBUG
  for(const auto& cell : standardCells.getCombCells()) {
    std::cout << CellType::get(cell).getName() << std::endl;
  }
#endif // UTOPIA_DEBUG
  return true;
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  checkLibParser(techLibPath / "sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  checkLibParser(techLibPath / "sky130_fd_sc_hd__ff_100C_1v65.lib");
}

} // namespace eda::gate::techmapper
