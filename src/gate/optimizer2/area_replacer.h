//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/replacer.h"

namespace eda::gate::optimizer2 {

/**
 * @brief Implements a replacing in the subnet for the area optimization.
 */
class AreaReplacer : public ReplacerBase {
public:
  /**
   * @brief Area optimizer constructor for subnet builder.
   * @param subnetBuilder Subnet for replacing.
   */
  AreaReplacer(SubnetBuilder &subnetBuilder) : ReplacerBase(subnetBuilder) {}

  void replace(SubnetFragment lhs, SubnetID rhs) override {
    const auto &lhsSubnet = Subnet::get(lhs.subnetID);
    const auto &rhsSubnet = Subnet::get(rhs);
    const auto lhsOutId = lhsSubnet.size() - 1;
    const auto rhsOutId = rhsSubnet.size() - 1;

    if (lhsOutId != rhsOutId) {
      lhs.entryMap[rhsOutId] = lhs.entryMap.at(lhsOutId);
      lhs.entryMap.erase(lhsOutId);
    }

    const auto gain = subnetBuilder.evaluateReplace(rhs, lhs.entryMap).size;
    if (gain <= 0) {
      return;
    }

    subnetBuilder.replace(rhs, lhs.entryMap);
  }
 
  void finalize() override {};
};

} // namespace eda::gate::optimizer2

