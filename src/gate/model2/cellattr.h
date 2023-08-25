//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Attributes
//===----------------------------------------------------------------------===//

class CellTypeAttr final : public Object<CellTypeAttr, CellTypeAttrID> {
  friend class Storage<CellTypeAttr>;

private:
  CellTypeAttr() {}
};

static_assert(sizeof(CellTypeAttr) == CellTypeAttrID::Size);

//===----------------------------------------------------------------------===//
// Cell Type Attributes Builder
//===----------------------------------------------------------------------===//

CellTypeAttrID makeCellTypeAttr() {
  return allocate<CellTypeAttr>();
}

} // namespace eda::gate::model
