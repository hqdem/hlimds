//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

namespace eda::gate::model {

std::ostream &operator <<(std::ostream &out, const Subnet &subnet) {
  using Link = Subnet::Link;

  const auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
  const auto InEntryLinks = Subnet::Cell::InEntryLinks;

  const auto cells = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = cells[i].cell;
    const auto &type = cell.getType();

    out << i << " <= " << type.getName() << "(";

    bool comma = false;
    for (size_t j = 0; j < cell.arity; j++) {
      if (comma) out << ", ";

      Link link;
      if (j < InPlaceLinks) {
        link = cell.link[j];
      } else {
        const auto k = (j - InPlaceLinks);
        link = cells[i + i + (k / InEntryLinks)].link[k % InEntryLinks];
      }

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
