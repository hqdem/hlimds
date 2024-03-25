//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/array.h"
#include "gate/model2/cell.h"

#include <cassert>

namespace eda::gate::model {

Cell::Cell(CellTypeID typeID, const LinkList &links):
    typeSID(typeID.getSID()), fanin(links.size()), fanout(0) {
  if (!links.empty()) {
    Array<uint64_t> array(links.size());
    for (size_t i = 0; i < links.size(); ++i) {
      array[i] = LinkEnd::pack(links[i]);
    }
    arrayID = array.getID();
  }
}

Cell::LinkList Cell::getLinks() const {
  if (!fanin) {
    return LinkList{};
  }

  LinkList links(fanin);
  Array<uint64_t> array(arrayID);
  for (size_t i = 0; i < fanin; ++i) {
    links[i] = LinkEnd::unpack(array[i]);
  }

  return links;
}

LinkEnd Cell::getLink(uint16_t port) const {
  Array<uint64_t> array(arrayID);
  return LinkEnd::unpack(array[port]);
}

void Cell::setLink(uint16_t port, const LinkEnd &source) {
  assert(port < fanin);
  Array<uint64_t> array(arrayID);
  assert(!LinkEnd::unpack(array[port]).isValid());
  array[port] = LinkEnd::pack(source);
}

} // namespace eda::gate::model
