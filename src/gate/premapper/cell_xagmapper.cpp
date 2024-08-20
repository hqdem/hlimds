//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/cell_xagmapper.h"

namespace eda::gate::premapper {

using Link          = CellXagMapper::Link;
using LinkList      = CellXagMapper::LinkList;
using SubnetBuilder = CellXagMapper::SubnetBuilder;

Link CellXagMapper::mapXor(const LinkList &links, bool &inv,
                           SubnetBuilder &builder) const {

  return builder.addCellTree(CellSymbol::XOR, links, 2);
}

} // namespace eda::gate::premapper
