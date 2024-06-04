//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/mapping.h"

#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace eda::gate::techmapper {
class AssemblySubnet {
  using Cell = model::Subnet::Cell;
  using Entry = model::Subnet::Entry;
  using LinkList = model::Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;

public:
  void assemble(
    const SubnetID subnetID, Mapping &mapping, SubnetBuilder &builder);

private:
  std::vector<EntryIndex> outID;
  std::vector<EntryIndex> inID;

  void findInOutCells(model::Array<Entry> entries);
  void addInputCells(Mapping &mapping, SubnetBuilder &builder);
  void addOutputCells(Mapping &mapping, SubnetBuilder &builder);
  LinkList createLinkList(
    const EntryIndex currentEntryIDX, Mapping &mapping);
  void processNode(
    const EntryIndex currentEntryIDX, Cell &currentCell,
    std::stack<EntryIndex> &stack, Mapping &mapping,
    SubnetBuilder &builder);
  void processLinks(
    const EntryIndex currentEntryIDX, std::stack<EntryIndex> &stack,
    std::unordered_set<EntryIndex> &visited, Mapping &mapping);
};
} // namespace eda::gate::techmapper
