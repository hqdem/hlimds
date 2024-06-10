//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/check_lib/aig/check_lib_for_aig.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::library {

std::pair<kitty::dynamic_truth_table, std::string> AIGCheckerLiberty::andTT() {
  return create_tt("1000", "A & B");
}

std::pair<kitty::dynamic_truth_table, std::string> AIGCheckerLiberty::invAndTT() {
  return create_tt("0010", "!A & B");
}

std::pair<kitty::dynamic_truth_table, std::string> AIGCheckerLiberty::andInvTT() {
  return create_tt("0100", "A & !B");
}

std::pair<kitty::dynamic_truth_table, std::string> AIGCheckerLiberty::invAndInvTT() {
  return create_tt("0001", "!A & !B");
}

std::pair<kitty::dynamic_truth_table, std::string> AIGCheckerLiberty::invTT() {
  return create_tt("0101", "!A");
}

std::vector<std::string> AIGCheckerLiberty::checkLiberty(
    std::unordered_map<kitty::dynamic_truth_table,
    std::vector<model::SubnetID>,
    DTTHash,
    DTTEqual> &truthTables) {
  std::vector<std::pair<kitty::dynamic_truth_table, std::string>> expressions =
      {andTT(), invAndTT(), andInvTT(), invAndInvTT(), invTT()};
  std::vector<std::string> missing;

  for (const auto& [expr, name] : expressions) {
    if (truthTables.find(expr) == truthTables.end()) {
      missing.push_back(name);
    }
  }
  return missing;
};

} // namespace eda::gate::library
