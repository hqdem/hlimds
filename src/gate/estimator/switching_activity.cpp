//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/switching_activity.h"

#include <algorithm>
#include <iterator>
#include <string>

void printDelimitedString(std::vector<std::string> &data, std::ostream &out) {
  std::move(data.begin(), data.end(),
      std::ostream_iterator<std::string>(out, ";"));
  out << std::endl;
}

namespace eda::gate::estimator {

void printSwitchActivity(const SwitchActivity &switchActivity,
                         const model::SubnetBuilder &builder,
                         std::ostream &out) {
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
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    const auto &cell = builder.getCell(*it);
    data = {
      std::to_string(*it),
      cell.getType().getName(),
      std::to_string(cell.arity),
      std::to_string(switchActivity.getSwitchProbability(*it)),
      std::to_string(switchActivity.getOnStateProbability(*it)),
    };
    if (printSwitches) {
      data.push_back(std::to_string(switchActivity.getSwitchesOn(*it)));
      data.push_back(std::to_string(switchActivity.getSwitchesOff(*it)));
    }
    printDelimitedString(data, out);
  }
}

} // namespace eda::gate::estimator
