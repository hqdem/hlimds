//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"

#include "gate/optimizer/rwdatabase.h"

#include "kitty/kitty.hpp"

#include <string>
#include <vector>
#include <list>

namespace eda::gate::tech_optimizer {

using SQLiteRWDatabase = eda::gate::optimizer::SQLiteRWDatabase;
using CellTypeID = eda::gate::model::CellTypeID;

struct Pin {
  Pin(const std::string &name, double cell_fall, double cell_rise,
      double fall_transition, double rise_transition);

  const std::string &getName() const;
  double getMaxDelay() const;

  private:
    const std::string name;
    double cell_fall;
    double cell_rise;
    double fall_transition;
    double rise_transition;
};

struct Cell {
  Cell(const std::string &name, const std::vector<Pin> &inputPins,
      kitty::dynamic_truth_table *truthTable, const std::string &realName);

  Cell(const std::string &name, const std::vector<Pin> &inputPins,
      kitty::dynamic_truth_table *truthTable, const std::string &realName, 
      double area);

  Cell(kitty::dynamic_truth_table *truthTable);

  const std::string &getName() const;
  double getArea() const; 
  kitty::dynamic_truth_table *getTruthTable() const;
  unsigned getInputPinsNumber() const;
  const Pin &getInputPin (uint inputPinNumber) const;

private:
  const std::string name;
  std::vector<Pin> inputPins;
  kitty::dynamic_truth_table *truthTable;
  std::string realName;
  double area;
};

struct LibraryCells {
  LibraryCells(const std::string &filename);

  std::vector<Cell*> cells;
  void initializeLibraryRwDatabase(SQLiteRWDatabase *arwdb, 
      std::unordered_map<std::string, CellTypeID> &cellTypeMap);

  std::list<CellTypeID> initializeLiberty();

  private:
  void readLibertyFile(const std::string &filename);
  
};

} // namespace eda::gate::tech_optimizer
