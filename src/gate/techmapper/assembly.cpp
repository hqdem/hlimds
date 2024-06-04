//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/assembly.h"

#include <cassert>
#include <iostream>

namespace eda::gate::techmapper {

void AssemblySubnet::assemble(
       const SubnetID subnetID, Mapping &mapping, SubnetBuilder &builder) {
  model::Subnet &subnet = model::Subnet::get(subnetID);
  auto entries = subnet.getEntries();
  findInOutCells(entries);

  addInputCells(mapping, builder);

  std::stack<EntryIndex> stack;
  std::unordered_set<EntryIndex> visited;

  for (const auto& out : outID) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    EntryIndex entryIndex = stack.top();
    assert(mapping.find(entryIndex) != mapping.end());
    if (mapping.at(entryIndex).cellID != ULLONG_MAX) {
      stack.pop();
      continue;
    }
    auto currentCell = entries[entryIndex].cell;
    processNode(entryIndex, currentCell, stack, mapping, builder);
    processLinks(entryIndex, stack, visited, mapping);
  }

  addOutputCells(mapping, builder);
  mapping.clear();
}

void AssemblySubnet::findInOutCells(model::Array<model::Subnet::Entry> entries) {
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (cell.isIn() || cell.getSymbol() == model::CellSymbol::IN) {
      inID.push_back(entryIndex);
    } else if (cell.isOut()|| cell.getSymbol() == model::CellSymbol::OUT) {
      outID.push_back(entryIndex);
    }
    entryIndex += cell.more;
  }
}

void AssemblySubnet::addInputCells(
       Mapping &mapping, SubnetBuilder &builder) {
  for (const auto idx : inID) {
    auto cellID = builder.addInput();
    mapping[idx].cellID = cellID.idx;
  }
}

void AssemblySubnet::addOutputCells(
       Mapping &mapping, SubnetBuilder &builder) {
  for (const auto idx : outID) {
    assert(mapping.find(idx) != mapping.end());
    auto inputs = mapping.at(idx).inputs;
    model::Subnet::Link link(mapping.at(*(inputs.begin())).cellID);
    auto cellID = builder.addOutput(link);
    mapping[idx].cellID = cellID.idx;
  }
}

model::Subnet::LinkList AssemblySubnet::createLinkList(
    const EntryIndex entryIndex, Mapping &mapping) {
  model::Subnet::LinkList linkList;
  for (const auto &idx : mapping.at(entryIndex).inputs) {
    if (mapping.find(idx) == mapping.end()) {
      std::cout << "Unable to find " << idx << " for " <<
        entryIndex << "!" << std::endl;
      assert(false);
    }
    if (mapping.at(idx).cellID == ULLONG_MAX) {
      return {}; // Return empty list to indicate not ready
    }
    model::Subnet::Link link(mapping.at(idx).cellID);
    linkList.push_back(link);
  }
  return linkList;
}

void AssemblySubnet::processNode(
       const EntryIndex entryIndex, Cell &currentCell,
       std::stack<EntryIndex> &stack, Mapping &mapping,
       SubnetBuilder &builder) {
  if (currentCell.isIn() || currentCell.getSymbol() == model::CellSymbol::IN) {
    stack.pop();
    return;
  }
  if (currentCell.isZero() || currentCell.isOne()) {
    auto symbol = currentCell.isZero() ? model::CellSymbol::ZERO :
                                         model::CellSymbol::ONE;
    auto cellID = builder.addCell(symbol);
    mapping[entryIndex].cellID = cellID.idx;
    stack.pop();
    return;
  }
  model::Subnet::LinkList linkList = createLinkList(entryIndex, mapping);
  if (!linkList.empty()) {
    if (!currentCell.isOut() ||
         currentCell.getSymbol() != model::CellSymbol::OUT) {
      auto cellID = builder.addSingleOutputSubnet(
        mapping.at(entryIndex).getSubnetID(), linkList);
      mapping[entryIndex].cellID = cellID.idx;
    }
    stack.pop();
  }
}

void AssemblySubnet::processLinks(
       const EntryIndex entryIndex, std::stack<EntryIndex> &stack,
       std::unordered_set<EntryIndex> &visited, Mapping &mapping) {
  for (const auto& link : mapping.at(entryIndex).inputs) {
    stack.push(link);
    /*if (visited.find(link) == visited.end()) {
      visited.insert(link);
      stack.push(link);
    } else {
    }*/
    //if (visited.insert(link).second) {
    // stack.push(link);
    //}
  }
}
} // namespace eda::gate::techmapper
