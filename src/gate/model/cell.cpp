//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/array.h"
#include "gate/model/cell.h"

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

//===----------------------------------------------------------------------===//
// Cell Validator
//===----------------------------------------------------------------------===//

#define OBJECT(cell) "Cell "
#define PREFIX(cell) OBJECT(cell) << ": "

#define VALIDATE(logger, prop, msg)\
  if (!(prop)) {\
    DIAGNOSE_ERROR(logger, msg);\
    return false;\
  }

#define VALIDATE_CELL(logger, cell, prop, msg)\
    VALIDATE(logger, prop, PREFIX(cell) << msg)

bool validateCell(const Cell &cell, diag::Logger &logger) {
  const auto &type = cell.getType();
  VALIDATE_CELL(logger, cell,
      validateCellType(type, logger),
      "[Invalid cell type]");
  VALIDATE_CELL(logger, cell,
      !type.isInNumFixed() || cell.getFanin() == type.getInNum(),
      "Incorrect number of inputs: " << cell.getFanin());

  const auto links = cell.getLinks();
  VALIDATE_CELL(logger, cell,
      links.size() == cell.getFanin(),
      "Incorrect number of links: " << links.size());

  for (const auto &link : links) {
    VALIDATE_CELL(logger, cell,
        validateSource(link, logger),
        "[Invalid link source]");
  }

  return true;
}

} // namespace eda::gate::model
