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

#define OBJECT(linkEnd) "Link"
#define PREFIX(linkEnd) OBJECT(linkEnd) << ": "

#define VALIDATE(logger, prop, msg)\
  if (!(prop)) {\
    DIAGNOSE_ERROR(logger, msg);\
    return false;\
  }

#define VALIDATE_LINKEND(logger, linkEnd, prop, msg)\
  VALIDATE(logger, prop, PREFIX(linkEnd) << msg)

bool validateSource(const LinkEnd &source, diag::Logger &logger) {
  VALIDATE_LINKEND(logger, source,
      source.isValid() && source.getCellID() != OBJ_NULL_ID,
      "Unconnected link source");

  const auto &cell = source.getCell();
  const auto &type = cell.getType();
  VALIDATE_LINKEND(logger, source,
      source.getPort() < type.getOutNum(),
      "Incorrect source pin: " << source.getPort() <<
      ", source cell has " << type.getOutNum() << "output pins");

  return true;
}

bool validateTarget(const LinkEnd &target, diag::Logger &logger) {
  VALIDATE_LINKEND(logger, target,
      target.isValid() && target.getCellID() != OBJ_NULL_ID,
      "Unconnected link target");

  const auto &cell = target.getCell();
  VALIDATE_LINKEND(logger, target,
      target.getPort() < cell.getFanin(),
      "Incorrect target pin: " << target.getPort() <<
      ", target cell has " << cell.getFanin() << "input pins");

  return true;
}

bool validateLink(const Link &link, diag::Logger &logger) {
  const auto &source = link.source;
  const auto &target = link.target;
  VALIDATE_LINKEND(logger, source, validateSource(source, logger),
      "[Incorrect source]");
  VALIDATE_LINKEND(logger, target, validateTarget(target, logger),
      "[Incorrect target]");
  return true;
}

} // namespace eda::gate::model
