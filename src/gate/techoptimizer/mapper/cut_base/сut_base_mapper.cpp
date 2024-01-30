//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

namespace eda::gate::tech_optimizer {
void CutBaseMapper::baseMap() {
  cutExtractor = new optimizer2::CutExtractor(&model::Subnet::get(subnetID), 6);
  findBest();
}
}