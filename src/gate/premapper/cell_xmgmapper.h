//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/premapper/cell_migmapper.h"

namespace eda::gate::premapper {

/// @brief Transforms a subnet to an XMG basis.
class CellXmgMapper final : public CellMigMapper {
public:

  CellXmgMapper(const std::string &name): CellMigMapper(name) {}

protected:
  Link mapXor(const LinkList &links, bool &inv,
              SubnetBuilder &builder) const override;
};

} // namespace eda::gate::premapper
