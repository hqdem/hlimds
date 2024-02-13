//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/array.h"
#include "gate/model2/cell.h"

namespace eda::gate::model {

Cell::Cell(CellTypeID typeID, const LinkList &links):
    typeSID(typeID.getSID()), fanin(links.size()), fanout(0) {
  if (fanin <= InPlaceLinks) {
    for (size_t i = 0; i < fanin; ++i) {
      data.link[i] = links[i];
    }
  } else {
    Array<uint64_t> array(fanin);
    for (size_t i = 0; i < links.size(); ++i) {
      array[i] = LinkEnd::pack(links[i]);
    }
    data.arrayID = array.getID();
  }
}

Cell::LinkList Cell::getLinks() const {
  LinkList links;

  if (fanin <= InPlaceLinks) {
    for (size_t i = 0; i < fanin; ++i) {
      links.push_back(data.link[i]);
    }
  } else {
    Array<uint64_t> array(data.arrayID);
    for (size_t i = 0; i < fanin; ++i) {
      links.push_back(LinkEnd::unpack(array[i]));
    }
  }

  return links;
}

LinkEnd Cell::getLink(uint16_t port) const {
  assert(port < fanin);
  if (fanin <= InPlaceLinks) {
    return data.link[port];
  }
  Array<uint64_t> array(data.arrayID);
  return LinkEnd::unpack(array[port]);
}

void Cell::setLink(uint16_t port, const LinkEnd &source) {
  assert(port < fanin);
  if (fanin <= InPlaceLinks) {
    assert(data.link[port].getCellID() == OBJ_NULL_ID);
    data.link[port] = source;
  } else {
    Array<uint64_t> array(data.arrayID);
    assert(LinkEnd::unpack(array[port]).getCellID() == OBJ_NULL_ID);
    array[port] = LinkEnd::pack(source);
  }
}

} // namespace eda::gate::model
