//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/cell.h"
#include "gate/model/link.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Link
//===----------------------------------------------------------------------===//

const Cell &LinkEnd::getCell() const {
  return Cell::get(getCellID());
}

//===----------------------------------------------------------------------===//
// Link Validator
//===----------------------------------------------------------------------===//

#define VALIDATE(prop) if (!(prop)) return false

bool validateLinkEnd(const LinkEnd &linkEnd) {
  const auto &type = linkEnd.getCell().getType();
  VALIDATE(linkEnd.isValid());
  VALIDATE(linkEnd.getCellID() != OBJ_NULL_ID);
  VALIDATE(linkEnd.getPort() <= type.getOutNum());
  return true;
}

bool validateLink(const Link &link) {
  const auto &source = link.source;
  const auto &target = link.target;
  VALIDATE(validateLinkEnd(source));
  VALIDATE(validateLinkEnd(target));
  return true;
}

} // namespace eda::gate::model
