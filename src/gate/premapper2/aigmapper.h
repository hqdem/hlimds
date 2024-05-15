//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/array.h"
#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/optimizer/transformer.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace eda::gate::premapper2 {

/// @brief Transforms a subnet to an AIG basis.
class AigMapper : public optimizer::SubnetTransformer {
public:
  using CellIdMap  = std::unordered_map<uint32_t, model::Subnet::Link>;
  using CellSymbol = eda::gate::model::CellSymbol;
  using Entries    = eda::gate::model::Array<eda::gate::model::Subnet::Entry>;
  using Link       = eda::gate::model::Subnet::Link;
  using LinkList   = eda::gate::model::Subnet::LinkList;
  using Subnet     = eda::gate::model::Subnet;

  std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) const override;

protected:
  Link mapCell(CellSymbol symbol, LinkList &links, bool &inv,
                 size_t n0, size_t n1, SubnetBuilder &builder) const;

  LinkList getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
                       const Subnet &oldSubnet, const Entries &entries,
                       size_t &n0, size_t &n1) const;

  Link mapIn(SubnetBuilder &builder) const;

  Link mapOut(const LinkList &links, SubnetBuilder &builder) const;

  Link mapVal(bool val, SubnetBuilder &builder) const;

  Link mapBuf(const LinkList &links, SubnetBuilder &builder) const;

  Link mapAnd(const LinkList &links, size_t n0, size_t n1,
                SubnetBuilder &builder) const;

  Link mapOr(LinkList &links, bool &inv, size_t n0, size_t n1,
               SubnetBuilder &builder) const;

  Link mapXor(LinkList &links, size_t n0, size_t n1,
                SubnetBuilder &builder) const;

  Link mapMaj(LinkList &links, bool &inv, size_t n0, size_t n1,
                SubnetBuilder &builder) const;

  Link addMaj3(LinkList &links, bool &inv, SubnetBuilder &builder) const;
};

} // namespace eda::gate::premapper2
