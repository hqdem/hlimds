//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/subnet_transformer.h"

#include <unordered_map>

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

  optimizer::SubnetBuilderPtr make(const SubnetID subnetID) const override;

protected:
  virtual Link mapCell(CellSymbol symbol, const LinkList &links, bool &inv,
                       size_t n0, size_t n1, SubnetBuilder &builder) const;

  virtual LinkList getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
                               const Subnet &oldSubnet, size_t &n0,
                               size_t &n1, SubnetBuilder &builder) const;

  virtual Link mapIn(SubnetBuilder &builder) const;

  virtual Link mapOut(const LinkList &links, SubnetBuilder &builder) const;

  virtual Link mapVal(bool val, SubnetBuilder &builder) const;

  virtual Link mapBuf(const LinkList &links, SubnetBuilder &builder) const;

  virtual Link mapAnd(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;
  
  virtual Link mapAnd(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const;

  virtual Link mapOr(const LinkList &links, bool &inv, size_t n0, size_t n1,
                     SubnetBuilder &builder) const;

  virtual Link mapOr(const LinkList &links, bool &inv,
                     SubnetBuilder &builder) const;

  virtual Link mapXor(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;

  virtual Link mapXor(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const;

  virtual Link mapMaj(const LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const;

  virtual Link mapMaj(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const;

  Link addMaj3(const LinkList &links, bool &inv, SubnetBuilder &builder) const;
  Link addMaj5(const LinkList &links, bool &inv, SubnetBuilder &builder) const;
};

} // namespace eda::gate::premapper
