//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/replacer.h"
#include "gate/optimizer/safe_passer.h"

namespace eda::gate::optimizer {

/**
 * @brief Implements a replacing in the subnet for the area optimization.
 */
class AreaReplacer : public ReplacerBase {
public:
  using SafePasser = eda::gate::optimizer::SafePasser;
  /**
   * @brief Area optimizer constructor for subnet builder.
   * @param subnetBuilder Subnet for replacing.
   * @param iter An iterator for the subnet.
   * @param delta Acceptable percentage of size deterioration.
   */
  AreaReplacer(SubnetBuilder &subnetBuilder,
               SafePasser &iter,
               double delta = 0.0)
      : ReplacerBase(subnetBuilder), iter(iter), delta(delta) {}

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
    if (gain < 0) {
      const auto &lhsSubnet = Subnet::get(lhs.subnetID);
      const double lhsSize = lhsSubnet.size();
      if ((-gain) / lhsSize * 100 > delta) {
        return;
      }
    }

    iter.replace(rhs, lhs.entryMap);
  }
 
  void finalize() override { };

private:
  SafePasser &iter;
  double delta;
};

} // namespace eda::gate::optimizer
