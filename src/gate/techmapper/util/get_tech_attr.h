//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

namespace eda::gate::tech_optimizer {

inline float getArea(model::SubnetID subnetID) {
  float area = 0;
  auto entr = model::Subnet::get(subnetID).getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entr); entryIndex++) {
    if (!entr[entryIndex].cell.isIn() && !entr[entryIndex].cell.isOut() ) {
      area += entr[entryIndex].cell.getType().getAttr().props.area;
    }
    entryIndex += entr[entryIndex].cell.more;
  }
  return area;
}

} // namespace eda::gate::tech_optimizer
