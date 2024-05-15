//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/techmapper/library/cell_db.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::techmapper {

class LibertyChecker {
public:
  virtual std::vector<std::string> checkLiberty(
      std::unordered_map<kitty::dynamic_truth_table, model::SubnetID, DTTHash, DTTEqual> &ttSubnet) = 0;

protected:
  inline std::pair<kitty::dynamic_truth_table, std::string>
      create_tt(const std::string& binary, const std::string& name) {
    kitty::dynamic_truth_table tt(2);
    kitty::create_from_binary_string(tt, binary);
    return {tt, name};
  }
};

} // namespace eda::gate::techmapper
