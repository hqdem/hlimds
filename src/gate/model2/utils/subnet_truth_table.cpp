//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"

namespace eda::gate::model {

kitty::dynamic_truth_table computeCare(const Subnet &subnet) {
  const size_t nSets = (1ull << subnet.getInNum());
  const auto tables = evaluate(subnet);

  kitty::dynamic_truth_table care(subnet.getOutNum());
  for (size_t i = 0; i < nSets; i++) {
    uint64_t careIndex = 0;
    for (size_t j = 0; j < tables.size(); ++j) {
      careIndex |= kitty::get_bit(tables[j], i) << j;
    }
    kitty::set_bit(care, careIndex);
  }
  return care;
}

} // namespace eda::gate::model
