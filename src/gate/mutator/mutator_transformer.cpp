//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/mutator/mutator_transformer.h"
#include "util/logging.h"

namespace eda::gate::mutator {
  MutatorTransformer::MutatorTransformer(Subnet &inputNet,
                                           SubnetBuilder &subnetBuilder,
                                           int numOfCells,
                                           CellIDList &cellIdList,
                                           CellSymbolList &function) :
                                           replacedCells(cellIdList),
                                           replacedFunc(function),
                                           numCells(numOfCells) {
    const auto &entries = inputNet.getEntries();
    findChildren(inputNet, entries);
    replacedCells = filterListCell(entries);
    for (size_t i = 0; i < entries.size(); i++) {
      Cell cell = entries[i].cell;
      auto isReplaceable = std::find(replacedCells.begin(), 
                                     replacedCells.end(), 
                                     i);
      if (isReplaceable != replacedCells.end()) {
        LinkList linkList = inputNet.getLinks(i);
        addMutatedCell(subnetBuilder, entries, i, linkList);
      } else {
        subnetBuilder.addCell(cell.getSymbol(), inputNet.getLinks(i));
      }
    }
  }

  bool MutatorTransformer::connectedWithOut(const EntryArray entries,
                                             const CellID &startCell) {
    CellIDList visited;
    std::list<CellID> queue;
    visited.push_back(startCell);
    queue.push_back(startCell);
    while (!queue.empty()) {
      CellID currCell = queue.front();
      queue.pop_front();
      CellIDList childCells = childCellList[currCell];
      for (CellID cellID : childCells) {
        auto isCellVisited = std::find(visited.begin(), visited.end(), 
                                       cellID);
        if (isCellVisited == visited.end()) {
          visited.push_back(cellID);
          queue.push_back(cellID);
          if (entries[cellID].cell.isOut()) {
            return true;
          }
        }
      }
    }
    return false;
  } 

  CellIDList MutatorTransformer::filterListCell(const EntryArray &entries) {
    CellIDList answerList;
    for (CellID cellID : replacedCells) {
      CellSymbol function = entries[cellID].cell.getSymbol();
      auto findFunc = std::find(replacedFunc.begin(), 
                                replacedFunc.end(), 
                                function);
      bool cellHasOut = connectedWithOut(entries, cellID);
      if (answerList.size() < numCells &&
          findFunc != replacedFunc.end() && 
          cellHasOut) {
        answerList.push_back(cellID);
      }
    }
    numMutated = answerList.size();
    return answerList;
  }

  void MutatorTransformer::addMutatedCell(SubnetBuilder &subnetBuilder,
                                           const EntryArray &entries,
                                           const CellID &cellID,
                                           const LinkList &linkList) {
    CellSymbol function = entries[cellID].cell.getSymbol();
    switch (function) {
      case CellSymbol::AND:
      case CellSymbol::XOR:
      case CellSymbol::NAND:
        subnetBuilder.addCell(CellSymbol::OR, linkList);
        return;
      case CellSymbol::OR:
      case CellSymbol::NOR:
        subnetBuilder.addCell(CellSymbol::AND, linkList);
        return;
      case CellSymbol::XNOR:
        subnetBuilder.addCell(CellSymbol::NOR, linkList);
        return;
      default:
        LOG_WARN << "Unexpected symbol: " << function;
        return;
    }
  }

  void MutatorTransformer::findChildren(Subnet &inputNet,
                                         const EntryArray &entries) {
    for (size_t i = 0; i < entries.size(); i++) {
      LinkList linkList = inputNet.getLinks(i);
      for (size_t j = 0; j < linkList.size(); j++) {
        CellID parent = linkList[j].idx;
        if (childCellList.find(parent) != childCellList.end()) {
          childCellList[parent].push_back(i);
        } else {
          childCellList.insert(std::pair<CellID, CellIDList>(parent, {i}));
        }
      }
    }
  }
} // namespace eda::gate::mutator
