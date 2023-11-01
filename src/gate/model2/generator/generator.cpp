//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/generator/generator.h"

namespace eda::gate::model {

Generator::Generator(const std::size_t nIn, const std::size_t nOut,
                     const unsigned seed):
  nIn(nIn), nOut(nOut), seed(seed), faninLow(1), faninHigh(CellType::AnyArity),
  hierarchical(false), nestingDepth(1), netCellsN(0u) {};

Generator::Generator(const std::size_t nIn, const std::size_t nOut,
                     const std::vector<CellSymbol> &netBase,
                     const unsigned seed):
  Generator(nIn, nOut, seed) {

  for (const auto cellSymb : netBase) {
    const auto cellTID = getCellTypeID(cellSymb);
    if (!isOperation(cellTID)) {
      throw std::invalid_argument(invalidCellTErrMsg);
    }
    CellType *cellT = &CellType::get(cellTID);
    this->netBase.push_back(cellTID);
    nInCellTIDs[cellT->getInNum()].push_back(cellTID);
  }
}

Generator::Generator(const std::size_t nIn, const std::size_t nOut,
                     const std::vector<CellTypeID> &netBase,
                     const unsigned seed):
  Generator(nIn, nOut, seed) {

  for (const auto &cellTID : netBase) {
    if (!isOperation(cellTID)) {
      throw std::invalid_argument(invalidCellTErrMsg);
    }
    CellType *cellT = &CellType::get(cellTID);
    this->netBase.push_back(cellTID);
    nInCellTIDs[cellT->getInNum()].push_back(cellTID);
  }
};

bool Generator::isOperation(const CellTypeID cellTID) const {
  return (!(cellTID == OBJ_NULL_ID || cellTID == CELL_TYPE_ID_IN ||
            cellTID == CELL_TYPE_ID_OUT || cellTID == CELL_TYPE_ID_ONE ||
            cellTID == CELL_TYPE_ID_ZERO));
}

void Generator::setFaninHigh(const uint16_t faninHigh) {
  setFaninLim(this->faninLow, faninHigh);
}

void Generator::setFaninLim(const uint16_t faninLow, const uint16_t faninHigh) {
  std::string faninBoundErr =
    "Fanin lower bound is greater than fanin upper bound.";
  std::string baseIrrelevantOps =
    "Generator basis has irrelevant operations.";
  if (faninLow > faninHigh) {
    throw std::invalid_argument(faninBoundErr);
  }

  for (std::size_t i = 0; i < netBase.size(); ++i) {
    const CellType *cellT = &CellType::get(netBase[i]);
    const uint16_t curOpInNum = cellT->getInNum();
    if (cellT->isAnyArity() && faninHigh < 2) {
      throw std::invalid_argument(baseIrrelevantOps);
    }
    if (!cellT->isAnyArity() &&
        (curOpInNum < faninLow || curOpInNum > faninHigh)) {
      throw std::invalid_argument(baseIrrelevantOps);
    }
  }
  this->faninLow = faninLow;
  this->faninHigh = faninHigh;
}

bool Generator::primInsOutsNotEmpty() const {
  return nIn && nOut;
}

bool Generator::isBounded(const uint16_t val, const uint16_t low,
                          const uint16_t high) const {

  return (val >= low && val <= high) ||
         (val == CellType::AnyArity && high >= 2);
}

bool Generator::canCreateNetCell() const {
  return faninLow <= nIn && faninHigh >= nIn && hierarchical && nestingDepth;
}

CellTypeID Generator::createNetCell() {
  if (!hierarchical) {
    return OBJ_NULL_ID;
  }
  nestingDepth--;
  setSeed(seed + 1);
  CellTypeID cellTID = makeCellType("net" + std::to_string(netCellsN),
                                    generate(), OBJ_NULL_ID, SOFT,
                                    CellProperties(1, 0, 0, 0, 0), nIn, nOut);
  nestingDepth++;
  netCellsN++;
  return cellTID;
}

bool Generator::canAddIn(const uint16_t cellNIn,
                         const std::size_t nSourceCells) const {

  if (cellNIn == CellType::AnyArity) {
    return false;
  }
  const auto nextInsCnt = nInCellTIDs.lower_bound(cellNIn + 1);

  if (nInCellTIDs.find(CellType::AnyArity) != nInCellTIDs.end() &&
      nSourceCells > 1 && cellNIn + 1 <= faninHigh) {
    return true;
  }

  return nextInsCnt != nInCellTIDs.end() &&
         nextInsCnt->first <= nSourceCells &&
         nextInsCnt->first <= faninHigh;
}

CellTypeID Generator::chooseCellType(const uint16_t cellNIn,
                                     const std::size_t nSourceCells) {

  uint16_t nInLowerBound = std::max(cellNIn, faninLow);
  uint16_t bottomNCellsToConnect = nSourceCells > 0xffff ? 0xffff :
                                   (uint16_t)nSourceCells;
  uint16_t nInUpperBound = std::min(bottomNCellsToConnect, faninHigh);
  if (nInLowerBound > nInUpperBound) {
    return OBJ_NULL_ID;
  }
  CellTypeID cellTID = OBJ_NULL_ID;
  std::size_t availNInNumber = 0;
  for (const auto &nInTIDs : nInCellTIDs) {
    if (isBounded(nInTIDs.first, nInLowerBound, nInUpperBound)) {
      availNInNumber++;
    }
  }
  if (!availNInNumber) {
    return OBJ_NULL_ID;
  }
  std::size_t nInNumber = std::rand() %
                          (availNInNumber + (canCreateNetCell() ? 1 : 0));
  if (nInNumber == availNInNumber) {
    return createNetCell();
  }
  for (const auto &nInTIDs : nInCellTIDs) {
    if (isBounded(nInTIDs.first, nInLowerBound, nInUpperBound)) {
      if (nInNumber) {
        nInNumber--;
        continue;
      }
      std::size_t cellTNumber = std::rand() % nInTIDs.second.size();
      cellTID = nInTIDs.second[cellTNumber];
      break;
    }
  }

  return cellTID;
}

NetID Generator::genEmptyNet() const {
  NetBuilder netBuilder;
  return netBuilder.make();
}

void Generator::setSeed(const unsigned seed) {
  this->seed = seed;
}

void Generator::setHierarchical(const bool hierarchical) {
  this->hierarchical = hierarchical;
}

void Generator::setNestingMax(const std::size_t nestMax) {
  nestingDepth = nestMax;
}

unsigned Generator::getSeed() const {
  return seed;
}

NetID Generator::generate() {
  std::srand(seed);
  if (!primInsOutsNotEmpty()) {
    return genEmptyNet();
  }

  return generateValid();
}

} // namespace eda::gate::model
