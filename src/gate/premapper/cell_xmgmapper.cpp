//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/cell_xmgmapper.h"

namespace eda::gate::premapper {

using Link          = CellXmgMapper::Link;
using LinkList      = CellXmgMapper::LinkList;
using SubnetBuilder = CellXmgMapper::SubnetBuilder;

Link CellXmgMapper::mapXor(const LinkList &links, bool &inv,
                           SubnetBuilder &builder) const {

  return builder.addCellTree(CellSymbol::XOR, links, 2);
}

} // namespace eda::gate::premapper
