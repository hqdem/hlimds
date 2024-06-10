//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/check_lib/check_lib.h"
#include "gate/model/subnet.h"

#include <unordered_map>
#include <vector>

namespace eda::gate::library {

class AIGCheckerLiberty : public LibertyChecker {
public:
  // Override the checkLiberty method
  std::vector<std::string> checkLiberty(
      std::unordered_map<kitty::dynamic_truth_table,
      std::vector<model::SubnetID>, DTTHash,
      DTTEqual> &truthTables) override;

private:
  std::pair<kitty::dynamic_truth_table, std::string> andTT();
  std::pair<kitty::dynamic_truth_table, std::string> invAndTT();
  std::pair<kitty::dynamic_truth_table, std::string> andInvTT();
  std::pair<kitty::dynamic_truth_table, std::string> invAndInvTT();
  std::pair<kitty::dynamic_truth_table, std::string> invTT();
};

} // namespace eda::gate::library
