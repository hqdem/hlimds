//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "kitty/kitty.hpp"

#include "gate/tech_mapper/library/cell.h"
#include "gate/tech_mapper/parser_lib_test.h"
#include "gate/tech_mapper/super_gate_generator/super_gate_generator.h"

#include "gtest/gtest.h"
#include <filesystem>

using namespace eda::gate::optimizer;
using namespace eda::gate::techMap;
using namespace eda::gate::model;

const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path libertyDirrect = homePath / "test" / "data" / "gate" / "tech_mapper";


bool checkLibParser(std::string liberty) {
  const std::string pathToLiberty = libertyDirrect / liberty;
  LibraryCells libraryCells(pathToLiberty);

  //for(const auto& cell : libraryCells.cells) {
    //std::cout << cell->getName() << std::endl;
  //}


  CircuitsGenerator circuitsGenerator;
  circuitsGenerator.setLibElementsList(libraryCells.cells);
  circuitsGenerator.initCircuit(2);
  circuitsGenerator.generateCircuits();

  for(const auto& cell : circuitsGenerator.getGeneratedNodes()) {
    kitty::print_binary(*(cell->getFunc()),std::cout);
  }
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