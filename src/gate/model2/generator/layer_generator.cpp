//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/generator/layer_generator.h"

namespace eda::gate::model {

/**
 * @brief Get a raised to the power n.
 * Get { a ^ n, 0 } if result < 0xffff. Otherwise get { a', k }, where
 * a' -- a raised to max power with result < 0xffff, k -- the number of residual
 * exponentiations.
 */
std::pair<std::size_t, std::size_t> binpow(std::size_t a, std::size_t n) {
  std::size_t res = 1;
  std::size_t residualExp = 1;
  while (n) {
    if (res * a >= 0xffff) {
      return { res, n * residualExp };
    }
    if (n & 1) {
      res *= a;
    }
    a *= a;
    residualExp *= 2;
    n >>= 1;
  }
  return { res, 0 };
}

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const std::vector<CellSymbol> &netBase,
                               const std::vector<std::size_t> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const CellSymbolList &netBase,
                               const std::vector<std::size_t> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const std::vector<CellSymbol> &netBase,
                               const std::size_t nLayers,
                               const uint16_t layerNCellsMin,
                               const uint16_t layerNCellsMax,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells({}), nLayers(nLayers),
  layerNCellsMin(layerNCellsMin), layerNCellsMax(layerNCellsMax) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const CellSymbolList &netBase,
                               const std::size_t nLayers,
                               const uint16_t layerNCellsMin,
                               const uint16_t layerNCellsMax,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells({}), nLayers(nLayers),
  layerNCellsMin(layerNCellsMin), layerNCellsMax(layerNCellsMax) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const std::vector<CellTypeID> &netBase,
                               const std::vector<std::size_t> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const CellTypeIDList &netBase,
                               const std::vector<std::size_t> &layerNCells,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells(layerNCells) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const std::vector<CellTypeID> &netBase,
                               const std::size_t nLayers,
                               const uint16_t layerNCellsMin,
                               const uint16_t layerNCellsMax,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells({}), nLayers(nLayers),
  layerNCellsMin(layerNCellsMin), layerNCellsMax(layerNCellsMax) {};

LayerGenerator::LayerGenerator(const std::size_t nIn,
                               const std::size_t nOut,
                               const CellTypeIDList &netBase,
                               const std::size_t nLayers,
                               const uint16_t layerNCellsMin,
                               const uint16_t layerNCellsMax,
                               const unsigned seed):
  Generator(nIn, nOut, netBase, seed), layerNCells({}), nLayers(nLayers),
  layerNCellsMin(layerNCellsMin), layerNCellsMax(layerNCellsMax) {};

std::string LayerGenerator::getName() const {
  return "LayerGenerator";
}

void LayerGenerator::setPrimIns(NetBuilder &netBuilder,
                                std::vector<CellID> &prevLayerCells,
                                std::vector<CellID> &addedCells) {
  for (std::size_t i = 0; i < nIn; ++i) {
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
  for (std::size_t j = 0; j < curLayerIns.size(); ++j) {
    Cell::LinkList &curCellIns = curLayerIns[j];

    if (curCellIns.empty()) {
      std::size_t inputNumber = std::rand() % prevLayerCells.size();
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
    if (outputs.size() == nOut) {
      return false;
    }
    outputs.push_back(prevLayerCell);
  }
  while (outputs.size() < nOut) {
    CellID outIn = addedCells[std::rand() % addedCells.size()];
    outputs.push_back(outIn);
  }
  for (const auto &output : outputs) {
    auto cellID = makeCell(OUT, output);
    netBuilder.addCell(cellID);
  }
  return true;
}

bool LayerGenerator::linkPrevLayer(const std::size_t cellsOnLayer,
                                   std::vector<Cell::LinkList> &curLayerIns,
                                   std::vector<CellID> &prevLayerCells,
                                   std::vector<CellID> &addedCells,
                                   std::vector<CellID> &outputs) {

  std::unordered_set<std::size_t> maxInputCells;
  std::shuffle(prevLayerCells.begin(), prevLayerCells.end(),
               std::default_random_engine(seed));

  for (const auto &prevLayerCell : prevLayerCells) {
    std::size_t curLayerCellToAddIn;
    do {
      curLayerCellToAddIn = std::rand() % cellsOnLayer;
      const auto curCellNIn = (uint16_t)curLayerIns[curLayerCellToAddIn].size();
      const auto nSourceCells = addedCells.size();
      if (!canAddIn(curCellNIn, nSourceCells)) {
        maxInputCells.insert(curLayerCellToAddIn);
      } else {
        break;
      }
    } while (maxInputCells.size() != cellsOnLayer);
    if (maxInputCells.size() == cellsOnLayer) {
      if (outputs.size() == nOut) {
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

  uint16_t inputsN;
  if (!cellT->isInNumFixed()) {
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

  while (curInputs.size() != inputsN) {
    std::size_t newInputInd = std::rand() % addedCells.size();
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

bool LayerGenerator::generateLayerNCells(const std::size_t nLayers,
                                         uint16_t layerNCellsMin,
                                         uint16_t layerNCellsMax) {

  std::srand(seed);
  layerNCells.resize(nLayers, 0);
  std::size_t prevLayerNCells = nIn;
  uint16_t netBaseNInMax = nInCellTIDs.rbegin()->first;
  uint16_t maxNIn = std::min(hierarchical ?
                             std::max(netBaseNInMax, (uint16_t)nIn) :
                             netBaseNInMax, faninHigh);
  // Maximum number of cells on the current layer if it is one primary output
  // in the net.
  std::pair<std::size_t, std::size_t> oneOutlayerNCellsMax =
      binpow(maxNIn, nLayers - 1);
  for (std::size_t i = 0; i < nLayers; ++i) {
    std::size_t curLayerNCellsMin = std::max((std::size_t)layerNCellsMin,
                                             prevLayerNCells / maxNIn +
                                             (prevLayerNCells % maxNIn ?
                                              1 : 0));
    layerNCellsMax = std::min((std::size_t)layerNCellsMax,
                              (oneOutlayerNCellsMax.second ? 0xffff :
                               oneOutlayerNCellsMax.first) * nOut);
    if (!oneOutlayerNCellsMax.second) {
      oneOutlayerNCellsMax.first /= maxNIn;
    } else {
      oneOutlayerNCellsMax.second--;
    }
    if (curLayerNCellsMin > layerNCellsMax) {
      return false;
    }
    layerNCells[i] = std::rand() % (layerNCellsMax - curLayerNCellsMin + 1) +
                     curLayerNCellsMin;
    prevLayerNCells = layerNCells[i];
  }
  return true;
}

NetID LayerGenerator::generateValid() {
  if (layerNCells.empty()) {
    if (!generateLayerNCells(nLayers, layerNCellsMin, layerNCellsMax)) {
      return genInvalidNet();
    }
  }

  std::vector<CellID> outputs;
  std::vector<CellID> prevLayerCells;
  std::vector<CellID> addedCells;
  NetBuilder netBuilder;

  setPrimIns(netBuilder, prevLayerCells, addedCells);
  for (std::size_t i = 0; i < layerNCells.size(); ++i) {
    std::vector<Cell::LinkList> curLayerIns(layerNCells[i]);

    if (!linkPrevLayer(layerNCells[i], curLayerIns, prevLayerCells,
                       addedCells, outputs) ||
        !setLayerCells(netBuilder, curLayerIns, prevLayerCells, addedCells)) {

      return genInvalidNet();
    }
  }

  if (!setPrimOuts(netBuilder, prevLayerCells, addedCells, outputs)) {
    return genInvalidNet();
  }
  return netBuilder.make();
}

} // namespace eda::gate::model
