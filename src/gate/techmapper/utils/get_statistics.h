//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/ppa_estimator.h"

namespace eda::gate::techmapper {

inline void printStatistics(model::SubnetID subnetID,
                            const library::SCLibrary &library) {
  size_t nWires = 0;
  size_t nCells = 0;

  std::unordered_map<std::string, int> statistic;
  for (const auto &cell : library.getCombCells()) {
    statistic[cell.name] = 0;
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
  for (const auto &pair : statistic) {
    if (pair.second != 0)
      std::cout << "     " <<
        std::left << std::setw(36) << pair.first <<
        std::right << std::setw(8) << pair.second << std::endl;
  }
  std::cout << "Design area: " << estimator::getArea(subnetID)
            << " um^2" << std::endl; // TODO export this scale from readcells

  std::cout << "Leakage power: "
            << estimator::getLeakagePower(subnetID, library)
            << " uW" << std::endl; // TODO the same

  std::cout << "Arrival time: " << estimator::getArrivalTime(subnetID, library)
            << " ns" << std::endl; // TODO the same
}

} // namespace eda::gate::techmapper
