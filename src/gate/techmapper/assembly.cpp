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

SubnetID AssemblySubnet::assemblySubnet(std::unordered_map<uint64_t, BestReplacement> *replacementMap,
                               SubnetID subnetID) {
   model::Subnet &subnet = model::Subnet::get(subnetID);
  bestReplacementMap = replacementMap;
  subnetBuilder = new model::SubnetBuilder;
  auto entries = subnet.getEntries();
  findInOutCells(entries);

  addInputCells();

  std::stack<EntryIndex> stack;
  std::unordered_set<EntryIndex> visited;

  for (const auto& out : outID) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    EntryIndex currentEntryIDX = stack.top();
    assert(bestReplacementMap->find(currentEntryIDX) != bestReplacementMap->end());
    if (bestReplacementMap->at(currentEntryIDX).cellIDInMappedSubnet != ULLONG_MAX) {
      stack.pop();
      continue;
    }
    auto currentCell = entries[currentEntryIDX].cell;
    processNode(currentEntryIDX,currentCell,stack);
    processLinks(currentEntryIDX, stack, visited);
  }

  addOutputCells();
  SubnetID mappedSubnetID = subnetBuilder->make();
  delete(subnetBuilder);
  bestReplacementMap->clear();
  return mappedSubnetID;
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

void AssemblySubnet::addInputCells() {
  for (const auto idx : inID) {
    auto cellID = subnetBuilder->addInput();
    (*bestReplacementMap)[idx].cellIDInMappedSubnet = cellID.idx;
  }
}

void AssemblySubnet::addOutputCells() {
  for (const auto idx : outID) {
    assert(bestReplacementMap->find(idx) != bestReplacementMap->end());
    auto input = bestReplacementMap->at(idx).entryIDxs;
    model::Subnet::Link link(bestReplacementMap->at
        (*(input.begin())).cellIDInMappedSubnet);
    auto cellID = subnetBuilder->addOutput(link);
    (*bestReplacementMap)[idx].cellIDInMappedSubnet = cellID.idx;
  }
}

model::Subnet::LinkList AssemblySubnet::createLinkList(EntryIndex currentEntryIDX) {
  model::Subnet::LinkList linkList;
  for (const auto &idx : bestReplacementMap->at(
      currentEntryIDX).entryIDxs) {
    assert(bestReplacementMap->find(idx) != bestReplacementMap->end());
    if (bestReplacementMap->at(idx).cellIDInMappedSubnet == ULLONG_MAX) {
      return {}; // Return empty list to indicate not ready
    }
    model::Subnet::Link link(bestReplacementMap->at(idx).cellIDInMappedSubnet);
    linkList.push_back(link);
  }
  return linkList;
}

void AssemblySubnet::processNode(EntryIndex currentEntryIDX,
                           model::Subnet::Cell &currentCell,
                           std::stack<EntryIndex> &stack) {
  if (currentCell.isIn() || currentCell.getSymbol() == model::CellSymbol::IN) {
    stack.pop();
    return;
  }
  if (currentCell.isZero() || currentCell.isOne()) {
    auto symbol = currentCell.isZero() ? model::CellSymbol::ZERO : model::CellSymbol::ONE;
    auto cellID = subnetBuilder->addCell(symbol);
    (*bestReplacementMap)[currentEntryIDX].cellIDInMappedSubnet = cellID.idx;
    stack.pop();
    return;
  }
  model::Subnet::LinkList linkList = createLinkList(currentEntryIDX);
  if (!linkList.empty()) {
    if (!currentCell.isOut() || currentCell.getSymbol() != model::CellSymbol::OUT) {
      auto cellID = subnetBuilder->addSingleOutputSubnet(bestReplacementMap->at(currentEntryIDX).subnetID, linkList);
      (*bestReplacementMap)[currentEntryIDX].cellIDInMappedSubnet = cellID.idx;
    }
    stack.pop();
  }
}
void AssemblySubnet::processLinks(EntryIndex currentEntryIDX,
                            std::stack<EntryIndex> &stack,
                            std::unordered_set<EntryIndex> &visited) {
  for (const auto& link : bestReplacementMap->at(currentEntryIDX).entryIDxs) {
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
