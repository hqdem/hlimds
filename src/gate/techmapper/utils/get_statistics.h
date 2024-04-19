//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/techmapper/library/liberty_manager.h"

#include <readcells/groups.h>

#include <filesystem>

namespace eda::gate::techmapper {

inline void printStatistics(model::SubnetID subnetID, std::string techLib) {
  int nWires = 0;
  int nCells = 0;

  std::unordered_map<std::string, int> statistic;
  for (const auto &cell : LibraryManager::get().getLibrary().getCells()) {
    statistic[std::string(cell.getName())] = 0;
  }
  const auto &entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); i++) {
    auto cellName = entries[i].cell.getType().getName();
    if (statistic.find(cellName) != statistic.end() ) {
      statistic[cellName] ++;
      nCells++;
    }
    nWires += entries[i].cell.getInPlaceLinks().size() + entries[i].cell.more;
    i += entries[i].cell.more;
  }
  std::cout << "Printing statistics:" << std::endl;
  std::cout << "   Number of wires: " << std::setw(10) << nWires << std::endl;
  std::cout << "   Number of cells: " << std::setw(10) << nCells << std::endl;
  for (const auto& pair : statistic) {
    if (pair.second != 0)
      std::cout << "     " <<
        std::left << std::setw(36) << pair.first <<
        std::right << std::setw(8) << pair.second << std::endl;
  }
}
} // namespace eda::gate::techmapper
