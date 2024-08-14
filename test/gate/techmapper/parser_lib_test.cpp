//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "util/env.h"

#include "gate/techmapper/utils/read_sdc.h"

#include "gtest/gtest.h"

#include <vector>

using path = std::filesystem::path;

const path home = eda::env::getHomePath();
const path techLibPath = home /
                     "test/data/gate/techmapper";

namespace eda::gate::library {
bool checkLibParser(std::string libertyPath) {
  SCLibrary library(libertyPath);
  std::cout << "Loaded Liberty: " << libertyPath << std::endl;// TODO

#ifdef UTOPIA_DEBUG
  for(const auto& cell : SCLibrary::get().getCombCells()) {
    std::cout << CellType::get(cell).getName() << std::endl;
  }
#endif // UTOPIA_DEBUG
  return true;
}

const std::tuple<float, float, float> checkSDCParser(std::string sdcPath) {
  const auto constraints = techmapper::parseSDCFile(sdcPath);
#ifdef UTOPIA_DEBUG
  std::cout << "Max delay: " << std::get<0>(constraints) << std::endl;
  std::cout << "Max area: " << std::get<1>(constraints) << std::endl;
  std::cout << "Max power: " << std::get<2>(constraints) << std::endl;
#endif // UTOPIA_DEBUG
  return constraints;
}


TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  checkLibParser(techLibPath / "sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  checkLibParser(techLibPath / "sky130_fd_sc_hd__ff_100C_1v65.lib");
}

TEST(ReadSDCTest, test_100) {
  const auto &constraints = checkSDCParser(techLibPath / "test.sdc");

  ASSERT_EQ(std::get<0>(constraints), 100);
  ASSERT_EQ(std::get<1>(constraints), 100);
  ASSERT_EQ(std::get<2>(constraints), 100);
}

} // namespace eda::gate::techmapper
