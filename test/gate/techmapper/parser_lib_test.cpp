//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "kitty/kitty.hpp"

#include "gate/model/gnet_test.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/techmapper/library/cell.h"
#include "util/logging.h"

#include "gtest/gtest.h"
#include <filesystem>

using namespace eda::gate::optimizer;
using namespace eda::gate::techmapper;
using namespace eda::gate::model;

const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path libertyDirrect = homePath / "test" / "data" / "gate" / "techmapper";


bool checkLibParser(std::string liberty) {
  const std::string pathToLiberty = libertyDirrect / liberty;
  LibraryCells libraryCells(pathToLiberty);

#ifdef UTOPIA_DEBUG
  for(const auto& cell : libraryCells.cells) {
    std::cout << cell->getName() << std::endl;
  }
#endif

  return true;
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_n40C_1v95) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(libertyDirrect.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest, sky130_fd_sc_hd__ff_100C_1v65) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
  checkLibParser(libertyDirrect.string() + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
}
