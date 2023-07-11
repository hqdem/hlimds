//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/rwdatabase.h"
#include "kitty/kitty.hpp"

#include <string>
#include <vector>
namespace eda::gate::techMap {
using SQLiteRWDatabase = eda::gate::optimizer::SQLiteRWDatabase;
struct Pin {
  public:
    Pin(const std::string name, double cell_fall, double cell_rise,
        double fall_transition, double rise_transition) :
      name(name), cell_fall(cell_fall), cell_rise(cell_rise),
      fall_transition(fall_transition), rise_transition(rise_transition) {}

    const std::string &getName() { return name; }

  private:
    const std::string name;
    double cell_fall;
    double cell_rise;
    double fall_transition;
    double rise_transition;

  public:
    double getMaxdelay() {
      double riseDelay = cell_rise + rise_transition;
      double fallDelay = cell_fall + fall_transition;
      return (riseDelay >= fallDelay ? riseDelay : fallDelay);
    }
};

struct Cell {
  Cell(const std::string name, const std::vector<Pin> inputPins,
      kitty::dynamic_truth_table *truthTable) :
    name(name), inputPins(inputPins), truthTable(truthTable) {}

  Cell(const std::string name, const std::vector<Pin> inputPins,
      kitty::dynamic_truth_table *truthTable, double area) :
    name(name), inputPins(inputPins), truthTable(truthTable), area(area) {}

  Cell(kitty::dynamic_truth_table *truthTable) :
    name(""), inputPins({}), truthTable(truthTable) {} // TODO: should we extract inputs number from TT?

  const std::string getName() { return name; }

  double getArea() { return area; }

  kitty::dynamic_truth_table* getTruthTable() { return truthTable; }

  unsigned getInputPinsNumber() { return inputPins.size(); }

  Pin &getInputPin (uint inputPinNumber) {
    assert(inputPinNumber < inputPins.size());
    return inputPins[inputPinNumber]; }

private:
  const std::string name;
  std::vector<Pin> inputPins;
  kitty::dynamic_truth_table *truthTable;
  double area;
};


struct LibraryCells {
  LibraryCells(std::string filename) {

    //system("python /home/");
    readLibertyFile(filename);
  }

  std::vector<Cell*> cells;
  void initializeLibraryRwDatabase(SQLiteRWDatabase *arwdb);

  private:
  void readLibertyFile(std::string filename);
};

//void initializeLibraryRwDatabase(SQLiteRWDatabase *arwdb);
} // namespace eda::gate::techMap
