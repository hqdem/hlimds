//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

namespace eda::gate::model {

std::pair<uint32_t, uint32_t> Subnet::getPathLength() const {
  uint32_t minLength = nCell, maxLength = 0;
  std::vector<uint32_t> min(nCell), max(nCell);

  const auto cells = getEntries();
  for (size_t i = 0; i < nCell; ++i) {
    const auto &cell = cells[i].cell;

    if (cell.isIn()) {
      min[i] = max[i] = 0;
    } else {
      min[i] = nCell; max[i] = 0;

      for (size_t j = 0; j < cell.arity; ++j) {
        auto link = getLink(i, j);
        min[i] = std::min(min[i], min[link.idx]);
        max[i] = std::max(max[i], max[link.idx]);
      }

      if (!cell.isPO()) {
        min[i]++; max[i]++;
      }
    }

    if (cell.isOut()) {
      minLength = std::min(minLength, min[i]);
      maxLength = std::max(maxLength, max[i]);
    }
  }

  return {minLength, maxLength};
}

std::ostream &operator <<(std::ostream &out, const Subnet &subnet) {
  const auto cells = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = cells[i].cell;
    const auto &type = cell.getType();

    out << i << " <= " << type.getName();
    if (cell.in) {
      out << (cell.dummy ? "[dummy]" : "[input]");
    }
    out << "(";

    bool comma = false;
    for (size_t j = 0; j < cell.arity; ++j) {
      if (comma) out << ", ";

      auto link = subnet.getLink(i, j);

      if (link.inv) out << "~";
      out << link.idx << "." << link.out;

      comma = true;
    }

    out << ");" << std::endl;
    i += cell.more;
  }

  return out;
}

} // namespace eda::gate::model
