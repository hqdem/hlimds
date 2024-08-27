//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/transformer.h"

#include <unordered_map>

namespace eda::gate::premapper {

/// @brief Interface class for premappers that map one cell at a time.
class CellPremapper : public optimizer::SubnetTransformer {
public:
  using CellIdMap = std::unordered_map<uint32_t, model::Subnet::Link>;
  using CellSymbol = model::CellSymbol;
  using Entries = model::Array<eda::gate::model::Subnet::Entry>;
  using Link = model::Subnet::Link;
  using LinkList = model::Subnet::LinkList;
  using Subnet = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
  using SubnetID = model::SubnetID;

  CellPremapper(const std::string &name): optimizer::SubnetTransformer(name) {}

  SubnetBuilderPtr map(const SubnetBuilderPtr &builder) const override;

protected:
  Link mapCell(CellSymbol symbol, const LinkList &links, bool &inv,
               size_t n0, size_t n1, SubnetBuilder &builder) const;

  LinkList getNewLinks(const CellIdMap &oldToNew, model::EntryID idx,
                       const Subnet &oldSubnet, size_t &n0, size_t &n1,
                       SubnetBuilder &builder) const;

  virtual Link mapIn(SubnetBuilder &builder) const;

  virtual Link mapOut(const LinkList &links, SubnetBuilder &builder) const;

  virtual Link mapVal(bool val, SubnetBuilder &builder) const;

  virtual Link mapBuf(const LinkList &links, SubnetBuilder &builder) const;

  virtual Link mapAnd(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;
  
  virtual Link mapAnd(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const = 0;

  virtual Link mapOr(const LinkList &links, bool &inv, size_t n0, size_t n1,
                     SubnetBuilder &builder) const;

  virtual Link mapOr(const LinkList &links, bool &inv,
                     SubnetBuilder &builder) const = 0;

  virtual Link mapXor(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;

  virtual Link mapXor(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const = 0;

  virtual Link mapMaj(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;

  virtual Link mapMaj(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const = 0;
};

} // namespace eda::gate::premapper
