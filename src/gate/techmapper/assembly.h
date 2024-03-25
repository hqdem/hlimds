//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techmapper/mapper/bestReplacement.h"

#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace eda::gate::tech_optimizer {
class AssemblySubnet {
public:
  SubnetID assemblySubnet(std::unordered_map<uint64_t, BestReplacement> *bestReplacementMap,
                                 SubnetID subnetID);
private:
  model::SubnetBuilder *subnetBuilder;
  std::unordered_map<uint64_t, BestReplacement> *bestReplacementMap;
  std::vector<EntryIndex> outID;
  std::vector<EntryIndex> inID;

  void findInOutCells(model::Array<model::Subnet::Entry> entries);
  void addInputCells();
  void addOutputCells();
  model::Subnet::LinkList createLinkList(EntryIndex currentEntryIDX);
  void processNode(EntryIndex currentEntryIDX,
                   model::Subnet::Cell &currentCell,
                   std::stack<EntryIndex> &stack);
  void processLinks(EntryIndex currentEntryIDX,
                    std::stack<EntryIndex> &stack,
                    std::unordered_set<EntryIndex> &visited);
};
} // namespace eda::gate::tech_optimizer
