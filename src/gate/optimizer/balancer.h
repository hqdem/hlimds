//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/transformer.h"

#include <map>

namespace eda::gate::optimizer {

class Balancer final : public SubnetInPlaceTransformer {
public:
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = eda::gate::model::SubnetID;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using EntryID = model::EntryID;

  Balancer(const std::string &name): SubnetInPlaceTransformer(name) {}

  void transform(const std::shared_ptr<SubnetBuilder> &builder) const override;

private:
  void updateDepth(const EntryID entryID) const;

  /// Starts balancing on the passed entryID.
  uint32_t balanceOnEntry(SubnetBuilder &builder, const EntryID entryID) const;

  /// Moves associative operation to the left/right, depending on what type of
  /// iterator is provided (reverse iterator or common iterator).
  template<typename Iter>
  void moveOp(
      SubnetBuilder &builder,
      const EntryID entryID,
      const Iter &operIter,
      const Iter &inputsBegin,
      const Iter &inputsEnd,
      const Iter &dInputsBegin,
      const Iter &dInputsEnd) const;

  /// Moves associative operation to the left/right, depending on what type of
  /// iterator is provided, while the depth of upper cell is not increasing.
  template<typename Iter>
  void moveOpToLim(
      SubnetBuilder &builder,
      const EntryID entryID,
      Iter operIter,
      Iter inputsBegin,
      Iter inputsEnd,
      Iter dOpInputsBegin,
      Iter dOpInputsEnd) const;

  /// Checks if it is possible to balance operations using associativity
  /// property.
  bool canBalanceAssoc(
      const SubnetBuilder &builder,
      const EntryID entryID1,
      const EntryID entryID2) const;

  /// Checks if it is possible to balance operations using complementary
  /// associativity property.
  bool canBalanceCompl(
      const SubnetBuilder &builder,
      const EntryID entryID1,
      const EntryID entryID2,
      const EntryID entryID3) const;

  /// Checks if it is possible to balance operations using associativity or
  /// complementary associativity.
  bool canBalance(
      const SubnetBuilder &builder,
      const EntryID entryID1,
      const EntryID entryID2,
      const EntryID entryID3) const;

  /// Moves all associative input operations left while the depth of the entryID
  /// cell is not increasing.
  void moveAllOpsLToLim(SubnetBuilder &builder, const EntryID entryID) const;

  /// Moves all associative input operations right while the depth of the
  /// entryID cell is not increasing.
  void moveAllOpsRToLim(SubnetBuilder &builder, const EntryID entryID) const;

  /// Implements associative balancing for operations.
  void balanceAssoc(SubnetBuilder &builder, const EntryID entryID) const;

  /// Implements associative balancing for operations with commutative property.
  void balanceCommutAssoc(SubnetBuilder &builder, const EntryID entryID) const;

  /// Implements complementary associative balancing. In the current version it
  /// is only MAJ cell type.
  void balanceComplAssoc(SubnetBuilder &builder, const EntryID entryID) const;
};

} // namespace eda::gate::optimizer
