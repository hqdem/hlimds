//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

namespace eda::gate::model {

class CellTypeAttr final {
public:
  using ID = CellTypeAttrID;
};

static_assert(sizeof(CellTypeAttr) == CellTypeAttrID::Size);

} // namespace eda::gate::model
