//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_checking.h"

namespace eda::gate::model::utils {

bool checkArity(const model::Subnet &subnet, uint16_t arity) {
  const auto &entries = subnet.getEntries();

  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = entries[i].cell;

    bool inOutput = cell.isIn()   || cell.isOut();
    bool constant = cell.isZero() || cell.isOne();

    if (!inOutput && !constant && (cell.arity > arity)) {
      return false;
    }
    i += cell.more;
  }
  return true;
}

} // namespace eda::gate::model::utils
