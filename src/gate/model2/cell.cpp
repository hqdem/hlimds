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

LinkEnd &Cell::getLink(uint16_t port) {
  assert(port < fanin);
  if (port <= InPlaceLinks) {
    return data.link[port];
  }
  Array<uint64_t> array(data.arrayID);
  return reinterpret_cast<LinkEnd&>(array[port]);
}

const LinkEnd &Cell::getLink(uint16_t port) const {
  assert(port < fanin);
  if (port <= InPlaceLinks) {
    return data.link[port];
  }
  Array<uint64_t> array(data.arrayID);
  return reinterpret_cast<const LinkEnd&>(array[port]);
}

} // namespace eda::gate::model
