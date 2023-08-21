//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/storage.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Attributes
//===----------------------------------------------------------------------===//

class CellTypeAttr final {
  friend class Storage<CellTypeAttr>;

public:
  using ID = CellTypeAttrID;

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
