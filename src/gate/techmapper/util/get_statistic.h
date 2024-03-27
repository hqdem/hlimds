//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <filesystem>

namespace eda::gate::tech_optimizer {

inline void printStatistic(model::SubnetID subnetID, std::string file_name) {
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path filePath = homePath / file_name;

  TokenParser tokParser;
  FILE *file = fopen(filePath.generic_string().c_str(), "rb");
  Group *ast = tokParser.parseLibrary(file,
                                      filePath.generic_string().c_str());
  Library lib;
  AstParser parser(lib, tokParser);
  parser.run(*ast);
  fclose(file);

  int nWires = 0;
  int nCells = 0;

  std::unordered_map<std::string, int> statistic;
  for (const auto& cell : lib.getCells()) {
    statistic[std::string(cell.getName())] = 0;
  }
  const auto &entries = model::Subnet::get(subnetID).getEntries();
  for (int i = 0; i < entries.size(); i++) {
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
} // namespace eda::gate::tech_optimizer
