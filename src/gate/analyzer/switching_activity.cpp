//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/switching_activity.h"

#include <algorithm>
#include <iterator>
#include <string>

void printDelimitedString(std::vector<std::string> &data, std::ostream &out) {
  std::move(data.begin(), data.end(),
      std::ostream_iterator<std::string>(out, ";"));
  out << std::endl;
}

namespace eda::gate::analyzer {

void printSwitchActivity(const SwitchActivity &switchActivity,
                         const model::Subnet &subnet, std::ostream &out) {
  out << "Simulation ticks: " << switchActivity.getTicks() << std::endl;
  std::vector<std::string> data{
    "ID",
    "Gate",
    "Arity",
    "SwitchActivity",
    "OnStateProbability",
    "SwitchesOn",
    "SwitchesOff"
  };
  printDelimitedString(data, out);
  bool printSwitches{switchActivity.getTicks() > 0};
  const auto &entries = subnet.getEntries();
  for (size_t i{0}; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    data = {
      std::to_string(i),
      cell.getType().getName(),
      std::to_string(cell.arity),
      std::to_string(switchActivity.getSwitchProbability(i)),
      std::to_string(switchActivity.getOnStateProbability(i)),
    };
    if (printSwitches) {
      data.push_back(std::to_string(switchActivity.getSwitchesOn(i)));
      data.push_back(std::to_string(switchActivity.getSwitchesOff(i)));
    }
    printDelimitedString(data, out);
    i += cell.more;
  }
}

} // namespace eda::gate::analyzer
