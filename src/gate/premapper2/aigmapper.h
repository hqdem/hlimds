//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/array.h"
#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/transformer/subnet_transformer.h"

#include <unordered_map>
#include <unordered_set>

namespace eda::gate::premapper2 {

/// @brief Transforms a subnet to an AIG basis.
class AigMapper : public transformer::SubnetTransformer {
public:
  using Builder    = eda::gate::model::SubnetBuilder;
  using CellIdMap  = std::unordered_map<size_t, size_t>;
  using CellSymbol = eda::gate::model::CellSymbol;
  using Entries    = eda::gate::model::Array<eda::gate::model::Subnet::Entry>;
  using InvCells   = std::unordered_set<size_t>;
  using Link       = eda::gate::model::Subnet::Link;
  using LinkList   = eda::gate::model::Subnet::LinkList;
  using Subnet     = eda::gate::model::Subnet;
  using SubnetID   = eda::gate::model::SubnetID;

  SubnetID transform(SubnetID id) override;

protected:
  size_t mapCell(CellSymbol symbol, LinkList &links, bool &inv,
                 size_t n0, size_t n1, Builder &builder) const;

  LinkList getNewLinks(const CellIdMap &oldToNew, size_t idx,
                       const Subnet &oldSubnet, const Entries &cells,
                       size_t &n0, size_t &n1, InvCells &toInvert) const;

  size_t mapIn(Builder &builder) const;

  size_t mapOut(const LinkList &links, Builder &builder) const;

  size_t mapVal(bool val, Builder &builder) const;

  size_t mapBuf(const LinkList &links, Builder &builder) const;

  size_t mapAnd(const LinkList &links, size_t n0, size_t n1,
                Builder &builder) const;

  size_t mapOr(LinkList &links, bool &inv, size_t n0, size_t n1,
               Builder &builder) const;

  size_t mapXor(LinkList &links, size_t n0, size_t n1,
                Builder &builder) const;

  size_t mapMaj(LinkList &links, bool &inv, size_t n0, size_t n1,
                Builder &builder) const;

  size_t addMaj3(LinkList &links, bool &inv, Builder &builder) const;
};

} // namespace eda::gate::premapper2
