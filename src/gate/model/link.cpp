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

const Cell &LinkEnd::getCell() const {
  return Cell::get(getCellID());
}

} // namespace eda::gate::model
