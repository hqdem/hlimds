//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/premapper/cell_premapper.h"

namespace eda::gate::premapper {

/// @brief Transforms a subnet to an AIG basis.
class CellAigMapper : public CellPremapper {
public:

  CellAigMapper(const std::string &name): CellPremapper(name) {}

protected:
  Link mapAnd(const LinkList &links, bool &inv,
              SubnetBuilder &builder) const override;

  Link mapOr(const LinkList &links, bool &inv,
             SubnetBuilder &builder) const override;

  Link mapXor(const LinkList &links, bool &inv,
              SubnetBuilder &builder) const override;

  Link mapMaj(const LinkList &links, bool &inv,
              SubnetBuilder &builder) const override;

  Link addMaj3(const LinkList &links, bool &inv, SubnetBuilder &builder) const;
  Link addMaj5(const LinkList &links, bool &inv, SubnetBuilder &builder) const;
};

} // namespace eda::gate::premapper
