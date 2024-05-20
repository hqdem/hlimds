//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/transformer.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace eda::gate::premapper {

/// @brief Transforms a subnet to an AIG basis.
class AigMapper final : public optimizer::SubnetTransformer {
public:
  using CellIdMap = std::unordered_map<uint32_t, model::Subnet::Link>;
  using CellSymbol = model::CellSymbol;
  using Entries = model::Array<eda::gate::model::Subnet::Entry>;
  using Link = model::Subnet::Link;
  using LinkList = model::Subnet::LinkList;
  using Subnet = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID = model::SubnetID;

  AigMapper(const std::string &name): optimizer::SubnetTransformer(name) {}

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

} // namespace eda::gate::premapper
