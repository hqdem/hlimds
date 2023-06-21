//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/library/cell.h"
#include "gate/tech_mapper/super_gate_generator/generateBestCircuits.h"
#include "gate/tech_mapper/parser_lib_test.h"

#include "gtest/gtest.h"
#include <filesystem>

using namespace eda::gate::optimizer;
using namespace eda::gate::model;

const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path libertyDirrect = homePath / "test" / "data" / "gate" / "tech_mapper";

void initializeLibraryRwDatabase(std::vector<Cell*> &cells, SQLiteRWDatabase *arwdb){
  for(auto& cell : cells) {
    uint64_t truthTable = 0;

    if (cell->getInputPinsNumber() == 0 ) {
      continue;
    }

    truthTable = 0b0000000000000000000000000000000000000000000000000000000000000000'0000000000000000000000000000000000000000000000000000000000000000;
    uint64_t _1 = 1;
    for (uint64_t i = 0; i < 64; i++) {
      if (kitty::get_bit(*(cell->getTruthTable()), i % cell->getTruthTable()->num_bits())) {
        truthTable |= (_1 << i);
      }
    }

    Gate::SignalList inputs;
    Gate::Id outputId;

    std::shared_ptr<GNet> dummy = makeCustom(cell->getInputPinsNumber(), 
        inputs, outputId, cell->getName());

    BoundGNet::GateBindings bindings;
    std::vector<double> delay;

    for (unsigned int i = 0; i < cell->getInputPinsNumber(); i++) {
        bindings.push_back(inputs[i].node());

      
        Pin pin = cell->getInputPin(i);
        double pinDelay = pin.getMaxdelay();
        delay.push_back(pinDelay);
      }

    dummy->sortTopologically();

    BoundGNet::BoundGNetList bgl;
    BoundGNet bg {dummy, bindings, {}, delay, cell->getName(), cell->getArea()};
    bgl.push_back(bg);

    RWDatabase::TruthTable TT(truthTable);

    auto list = arwdb->get(truthTable);
    if (list.size() == 0) {
       arwdb->set(TT, bgl);
    } else {
      list.push_back(bg);
      arwdb->set(TT, list);
    }
  }
}

bool checkLibParser(std::string liberty) {
  const std::string pathToLiberty = libertyDirrect / liberty;
  LibraryCells libraryCells(pathToLiberty);

  for(const auto& cell : libraryCells.cells) {
    std::cout << cell->getName() << std::endl;
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