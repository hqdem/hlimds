//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/cell.h"
#include "gate/model2/list.h"

namespace eda::gate::model {

static_assert(sizeof(LinkEnd) == sizeof(ListID));

Cell::Cell(CellTypeID typeID, const LinkList &links):
    typeSID(typeID.getSID()), fanin(links.size()), fanout(0) {
  if (fanin <= InPlaceLinks) {
    for (auto i = 0u; i < fanin; ++i) {
      link[i] = links[i];
    }
  } else {
    List<uint64_t> list(fanin);
    for (auto i = links.begin(); i != links.end(); ++i) {
      list.push_back(LinkEnd::pack(*i));
    }
    link[0] = LinkEnd::unpack(list.getID());
  }
}

Cell::LinkList Cell::getLinks() const {
  LinkList links;

  if (fanin <= InPlaceLinks) {
    for (auto i = 0u; i < fanin; ++i) {
      links.push_back(link[i]);
    }
  } else {
    List<uint64_t> list(static_cast<ListID>(LinkEnd::pack(link[0])));
    for (auto i = list.begin(); i != list.end(); ++i) {
      links.push_back(LinkEnd::unpack(*i));
    }
  }

  return links;
}

} // namespace eda::gate::model
