//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/mutator/mutator.h"
#include "util/logging.h"

namespace eda::gate::mutator {

/// Check if cell is out
bool isOut(const EntryArray &entries, size_t &cellID) {
  return entries[cellID].cell.isOut();
}

/// Check if cell is in
bool isIn(const EntryArray &entries, size_t &cellID) {
  return entries[cellID].cell.isIn();
}
/// Makes list of cells and their cuts transformed into list of cells
CellIDList makeListCell(Subnet &net,
                        unsigned int numOfCuts,
                        CellIDList &cellIdList,
                        unsigned int cutSize) {
  if (net.size() == 0) {
    LOG_WARN << "Input Subnet is empty\n";
    return {};
  }
  CutExtractor cutExtractor(&net, cutSize);
  CellIDList answerList;
  const auto &entries = net.getEntries();
  if (cellIdList.empty()) {
    unsigned int count = 0;
    size_t cellID = net.size() - 1;
    while (count < numOfCuts && cellID >= 0) {
      if (!isOut(entries, cellID) && !isIn(entries, cellID)) {
        cellIdList.push_back(cellID);
        count++;
      }
      cellID--;
    }
  }
  for (size_t cellID : cellIdList) {
    if (!isOut(entries, cellID) && !isIn(entries, cellID)) {
      auto findCellID = std::find(answerList.begin(), 
                                  answerList.end(), 
                                  cellID);
      if (findCellID == answerList.end()) {
        answerList.push_back(cellID);
      }
      const auto &cuts = cutExtractor.getCuts(cellID);
      for (const auto &cut : cuts) {
        for (size_t cutCellId : cut.entryIDs) {
          auto findCell = std::find(answerList.begin(), 
                                    answerList.end(), 
                                    cutCellId);
          if (findCell == answerList.end()) {
            answerList.push_back(cutCellId);
          }
        }
      }
    }
  }
  return answerList;
}

/// Creates parameters for mutator transformer
bool paramForTransformer(MutatorMode mode,
                         Subnet &inputNet, 
                         unsigned int &number,
                         CellIDList &cellIdList,
                         unsigned int &cutSize) {
  if (mode == MutatorMode::CUT) {
    cellIdList = makeListCell(inputNet, number, cellIdList, cutSize);
    number = cellIdList.size();
  } else if (mode == MutatorMode::CELL) {
    for (size_t i = 0; i < inputNet.size(); i++) {
      cellIdList.push_back(i);
    }
  } else {
    return false;
  }
  return true;
}

//===------------------------------------------------------------------===//
// Static functions
//===------------------------------------------------------------------===// 

SubnetID Mutator::mutate(MutatorMode mode,
                          Subnet &inputNet,
                          CellIDList &cellIdList,
                          CellSymbolList function,
                          unsigned int cutSize) {
  SubnetBuilder subnetBuilder;
  if (mode == MutatorMode::CUT) {
    cellIdList = makeListCell(inputNet,
                              cellIdList.size(),
                              cellIdList,
                              cutSize);
  } else if (mode != MutatorMode::CELL) {
    LOG_WARN << "Unexpected flag : " << mode;
    return subnetBuilder.make();
  }
  MutatorTransformer transformer(inputNet,
                                  subnetBuilder,
                                  cellIdList.size(),
                                  cellIdList,
                                  function);
  return subnetBuilder.make();
}

SubnetID Mutator::mutate(MutatorMode mode,
                          Subnet &inputNet,
                          unsigned int num,
                          CellSymbolList function,
                          unsigned int cutSize) {
  SubnetBuilder subnetBuilder;
  CellIDList cellIdList;
  bool result = paramForTransformer(mode, 
                                    inputNet, 
                                    num, 
                                    cellIdList, 
                                    cutSize);
  if (!result) {
    LOG_WARN << "Unexpected flag : " << mode;
    return subnetBuilder.make();
  }
  MutatorTransformer transformer(inputNet,
                                  subnetBuilder,
                                  num,
                                  cellIdList,
                                  function);
  return subnetBuilder.make();
}

SubnetID Mutator::mutate(MutatorMode mode,
                          int &counter,
                          Subnet &inputNet,
                          unsigned int num,
                          CellSymbolList function,
                          unsigned int cutSize) {
  SubnetBuilder subnetBuilder;
  CellIDList cellIdList;
  bool result = paramForTransformer(mode, 
                                    inputNet, 
                                    num, 
                                    cellIdList, 
                                    cutSize);
  if (!result) {
    LOG_WARN << "Unexpected flag : " << mode;
    return subnetBuilder.make();
  }
  MutatorTransformer transformer(inputNet,
                                  subnetBuilder,
                                  num,
                                  cellIdList,
                                  function);
  counter = transformer.getNumMutatedCells();
  return subnetBuilder.make();
}

SubnetID Mutator::mutate(MutatorMode mode,
                          CellIDList &mutatedCells,
                          Subnet &inputNet,
                          unsigned int num,
                          CellSymbolList function,
                          unsigned int cutSize) {
  SubnetBuilder subnetBuilder;
  CellIDList cellIdList;
  bool result = paramForTransformer(mode, 
                                    inputNet, 
                                    num, 
                                    cellIdList, 
                                    cutSize);
  if (!result) {
    LOG_WARN << "Unexpected flag : " << mode;
    return subnetBuilder.make();
  }
  MutatorTransformer transformer(inputNet,
                                  subnetBuilder,
                                  num,
                                  cellIdList,
                                  function);
  mutatedCells = transformer.getMutatedCellsList();
  return subnetBuilder.make();
}

} // namespace eda::gate::mutator

