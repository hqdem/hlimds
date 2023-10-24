//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/generator/layer_generator.h"

namespace eda::gate::model {

LayerGenerator::LayerGenerator(const int nIn, const int nOut,
                               const std::vector<CellSymbol> &netBase,
                               const std::vector<int> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

LayerGenerator::LayerGenerator(const int nIn, const int nOut,
                               const std::vector<CellTypeID> &netBase,
                               const std::vector<int> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

std::string LayerGenerator::getName() const {
  return "LayerGenerator";
}

void LayerGenerator::setPrimIns(NetBuilder &netBuilder,
                                std::vector<CellID> &prevLayerCells,
                                std::vector<CellID> &addedCells) {
  for (int i = 0; i < nIn; ++i) {
    auto cellID = makeCell(IN);
    netBuilder.addCell(cellID);
    prevLayerCells.push_back(cellID);
    addedCells.push_back(cellID);
  }
}

bool LayerGenerator::setLayerCells(NetBuilder &netBuilder,
                                   std::vector<Cell::LinkList> &curLayerIns,
                                   std::vector<CellID> &prevLayerCells,
                                   std::vector<CellID> &addedCells) {
  std::vector<CellID> curLayerCells;
  for (int j = 0; j < (int)curLayerIns.size(); ++j) {
    Cell::LinkList &curCellIns = curLayerIns[j];

    if (curCellIns.empty()) {
      int inputNumber = std::rand() % prevLayerCells.size();
      curCellIns.push_back(LinkEnd(prevLayerCells[inputNumber]));
    }
    if (!setOp(curLayerCells, curCellIns, addedCells, netBuilder)) {
      return false;
    }
  }

  prevLayerCells = curLayerCells;
  for (const auto &curCell : curLayerCells) {
    addedCells.push_back(curCell);
  }
  return true;
}

bool LayerGenerator::setPrimOuts(NetBuilder &netBuilder,
                                 std::vector<CellID> &prevLayerCells,
                                 std::vector<CellID> &addedCells,
                                 std::vector<CellID> &outputs) {
  for (const auto &prevLayerCell : prevLayerCells) {
    if ((int)outputs.size() == nOut) {
      return false;
    }
    outputs.push_back(prevLayerCell);
  }
  while ((int)outputs.size() < nOut) {
    CellID outIn = addedCells[std::rand() % addedCells.size()];
    outputs.push_back(outIn);
  }
  for (const auto &output : outputs) {
    auto cellID = makeCell(OUT, output);
    netBuilder.addCell(cellID);
  }
  return true;
}

bool LayerGenerator::linkPrevLayer(const int cellsOnLayer,
                                   std::vector<Cell::LinkList> &curLayerIns,
                                   std::vector<CellID> &prevLayerCells,
                                   std::vector<CellID> &addedCells,
                                   std::vector<CellID> &outputs) {

  std::unordered_set<int> maxInputCells;
  std::shuffle(prevLayerCells.begin(), prevLayerCells.end(),
               std::default_random_engine(seed));

  for (const auto &prevLayerCell : prevLayerCells) {
    int curLayerCellToAddIn;
    do {
      curLayerCellToAddIn = std::rand() % cellsOnLayer;
      const int curLayerCellNIn = (int)curLayerIns[curLayerCellToAddIn].size();
      const int nSourceCells = (int)addedCells.size();
      if (!canAddIn(curLayerCellNIn, nSourceCells)) {
        maxInputCells.insert(curLayerCellToAddIn);
      } else {
        break;
      }
    } while ((int)maxInputCells.size() != cellsOnLayer);
    if ((int)maxInputCells.size() == cellsOnLayer) {
      if ((int)outputs.size() == nOut) {
        return false;
      }
      outputs.push_back(prevLayerCell);
      continue;
    }
    curLayerIns[curLayerCellToAddIn].push_back(LinkEnd(prevLayerCell));
  }

  return true;
}

void LayerGenerator::setInputs(Cell::LinkList &curInputs,
                               const CellTypeID cellTID,
                               std::vector<CellID> &addedCells) {

  const CellType *cellT = &CellType::get(cellTID);
  std::unordered_set<CellID, CellIDHash> curInputsSet;
  for (const auto &input : curInputs) {
    curInputsSet.insert(input.getCellID());
  }

  int inputsN;
  if (cellT->isAnyArity()) {
    const uint16_t lowerBound = std::max((uint16_t)2,
                                         std::max((uint16_t)curInputs.size(),
                                         faninLow));
    const uint16_t nSourceCells = addedCells.size() > CellType::AnyArity ?
                                  CellType::AnyArity :
                                  (uint16_t)addedCells.size();
    const uint16_t upperBound = std::min(nSourceCells, faninHigh);
    inputsN = std::rand() % (upperBound - lowerBound + 1) + lowerBound;
  } else {
    inputsN = cellT->getInNum();
  }

  while ((int)curInputs.size() != inputsN) {
    int newInputInd = std::rand() % addedCells.size();
    if (curInputsSet.find(addedCells[newInputInd]) == curInputsSet.end()) {
      curInputsSet.insert(addedCells[newInputInd]);
      curInputs.push_back(LinkEnd(addedCells[newInputInd]));
    }
  }
}

bool LayerGenerator::setOp(std::vector<CellID> &curLayerCells,
                           Cell::LinkList &curCellIns,
                           std::vector<CellID> &addedCells,
                           NetBuilder &netBuilder) {

  const CellTypeID cellTID = chooseCellType(curCellIns.size(),
                                            addedCells.size());
  if (cellTID == OBJ_NULL_ID) {
    return false;
  }
  setInputs(curCellIns, cellTID, addedCells);

  auto cellId = makeCell(cellTID, curCellIns);
  curLayerCells.push_back(cellId);
  netBuilder.addCell(cellId);

  return true;
}

NetID LayerGenerator::generateValid() {
  std::vector<CellID> outputs;
  std::vector<CellID> prevLayerCells;
  std::vector<CellID> addedCells;
  NetBuilder netBuilder;

  setPrimIns(netBuilder, prevLayerCells, addedCells);
  for (int i = 0; i < (int)layerNCells.size(); ++i) {
    std::vector<Cell::LinkList> curLayerIns(layerNCells[i]);

    if (!linkPrevLayer(layerNCells[i], curLayerIns, prevLayerCells,
                       addedCells, outputs) ||
        !setLayerCells(netBuilder, curLayerIns, prevLayerCells, addedCells)) {

      return genEmptyNet();
    }
  }

  if (!setPrimOuts(netBuilder, prevLayerCells, addedCells, outputs)) {
    return genEmptyNet();
  }
  return netBuilder.make();
}

} // namespace eda::gate::model
