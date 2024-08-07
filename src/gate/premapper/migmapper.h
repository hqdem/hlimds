//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/premapper/premapper.h"

namespace eda::gate::premapper {

/// @brief Transforms a subnet to an MIG basis.
class MigMapper final : public Premapper {
public:
  MigMapper(const std::string &name): optimizer::SubnetTransformer(name) {}

protected:
  virtual Link mapAnd(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const override;

  virtual Link mapOr(const LinkList &links, bool &inv,
                     SubnetBuilder &builder) const override;

  virtual Link mapXor(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const override;

  virtual Link mapMaj(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const override;

  Link addMajTree(CellSymbol symbol, const LinkList &links,
                  SubnetBuilder &builder) const;

  Link addMaj(CellSymbol symbol, const LinkList &links,
              SubnetBuilder &builder) const;

  Link addXor(const LinkList &links, SubnetBuilder &builder) const;

  Link addMaj5(const LinkList &links, SubnetBuilder &builder) const;
};

} // namespace eda::gate::premapper
