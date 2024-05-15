//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/generator/matrix_generator.h"

namespace eda::gate::model {

MatrixGenerator::MatrixGenerator(const std::size_t nCells,
                                 const std::size_t nIn,
                                 const std::size_t nOut,
                                 const std::vector<CellSymbol> &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

MatrixGenerator::MatrixGenerator(const std::size_t nCells,
                                 const std::size_t nIn,
                                 const std::size_t nOut,
                                 const CellSymbolList &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

MatrixGenerator::MatrixGenerator(const std::size_t nCells,
                                 const std::size_t nIn,
                                 const std::size_t nOut,
                                 const std::vector<CellTypeID> &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

MatrixGenerator::MatrixGenerator(const std::size_t nCells,
                                 const std::size_t nIn,
                                 const std::size_t nOut,
                                 const CellTypeIDList &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

std::string MatrixGenerator::getName() const {
  return "MatrixGenerator";
}

void MatrixGenerator::setPrimIns(std::unordered_set<std::size_t> &inputs) {
  while (inputs.size() != nIn) {
    inputs.insert(matrixNCells - inputs.size() - 1);
  }
}

bool MatrixGenerator::canMakeDrain(Matrix &m,
                                   const std::size_t columnN,
                                   CellToNIn &cellNIn) {

  for (std::size_t i = 0; i < matrixNCells; ++i) {
    const bool curRowHasIn = (*m)[i][columnN];
    const bool noLessInNOps = nInCellTIDs.find(cellNIn[i] - 1) ==
                              nInCellTIDs.end();
    const bool noAnyNInOps = nInCellTIDs.find(CellType::AnyArity) ==
                             nInCellTIDs.end();
    if (curRowHasIn && (noLessInNOps && (cellNIn[i] < 3 || noAnyNInOps))) {
      return false;
    }
  }
  return true;
}

void MatrixGenerator::setPrimOuts(Matrix &m,
                                  std::vector<std::size_t> &outputs,
                                  CellToNIn &cellNIn,
                                  CellIdxToCellType &cellIdxCellTID) {

  outputs.push_back(0);
  for (std::size_t k = 0; k < nOut - 1; ++k) {
    if (!(std::rand() % 2)) {
      continue;
    }
    assert(matrixNCells != 0);
    std::size_t j = std::rand() % matrixNCells;

    if (!canMakeDrain(m, j, cellNIn)) {
      continue;
    }
    for (std::size_t i = 0; i < matrixNCells; ++i) {
      if ((*m)[i][j]) {
        if (nInCellTIDs.find(cellNIn[i] - 1) != nInCellTIDs.end()) {
          std::size_t opN = std::rand() % nInCellTIDs[cellNIn[i] - 1].size();
          cellIdxCellTID[i] = nInCellTIDs[cellNIn[i] - 1][opN];
        } else {
          std::size_t opN = std::rand() %
            nInCellTIDs[CellType::AnyArity].size();
          cellIdxCellTID[i] = nInCellTIDs[CellType::AnyArity][opN];
        }
        cellNIn[i]--;
      }
      (*m)[i][j] = false;
    }
    outputs.push_back(j);
  }

  while (nOut > outputs.size()) {
    assert(matrixNCells != 0);
    outputs.push_back(std::rand() % matrixNCells);
  }
}

bool MatrixGenerator::setCellsOuts(Matrix &m, CellToNIn &cellNIn) {
  for (std::size_t j = 1; j < matrixNCells; ++j) {
    std::size_t availRowsN = std::min(j, matrixNCells - nIn);
    std::unordered_set<std::size_t> unavailRows;
    assert(availRowsN != 0);
    std::size_t i;
    do {
      i = std::rand() % availRowsN;
      if (!canAddIn(cellNIn[i], matrixNCells - i - 1)) {
        unavailRows.insert(i);
      } else {
        break;
      }
    } while (unavailRows.size() != availRowsN);
    if (unavailRows.size() == availRowsN) {
      return false;
    }

    (*m)[i][j] = true;
    cellNIn[i]++;
  }

  return true;
}

void MatrixGenerator::addInsForCell(const std::size_t rowN,
                                    Matrix &m,
                                    CellToNIn &cellNIn,
                                    CellIdxToCellType &cellIdxCellTID) {

  uint16_t neededCurNIn;
  const CellType *cellT = &CellType::get(cellIdxCellTID[rowN]);
  if (!cellT->isInNumFixed()) {
    const uint16_t lowerBound = std::max((uint16_t)2,
                                         std::max(cellNIn[rowN], faninLow));
    const uint16_t nSourceCells = matrixNCells - rowN - 1 > CellType::AnyArity ?
                                  CellType::AnyArity :
                                  (uint16_t)(matrixNCells - rowN - 1);
    const uint16_t upperBound = std::min(nSourceCells, faninHigh);
    assert(upperBound - lowerBound + 1 != 0);
    neededCurNIn = std::rand() % (upperBound - lowerBound + 1) + lowerBound;
  } else {
    neededCurNIn = cellT->getInNum();
  }
  while (cellNIn[rowN] < neededCurNIn) {

    assert((matrixNCells - rowN - 1) != 0);
    std::size_t j = std::rand() % (matrixNCells - rowN - 1) + rowN + 1;
    if (!(*m)[rowN][j]) {
      (*m)[rowN][j] = true;
      cellNIn[rowN] += (*m)[rowN][j] ? 1 : 0;
    }
  }
}

bool MatrixGenerator::setOp(const std::size_t i,
                            CellToNIn &cellNIn,
                            CellIdxToCellType &cellIdxCellTID) {

  CellTypeID cellTID = chooseCellType(cellNIn[i], matrixNCells - i - 1);
  if (cellTID == OBJ_NULL_ID) {
    return false;
  }
  cellIdxCellTID[i] = cellTID;

  return true;
}

bool MatrixGenerator::setOps(Matrix &m,
                             CellToNIn &cellNIn,
                             CellIdxToCellType &cellIdxCellTID) {

  if (!setCellsOuts(m, cellNIn)) {
    return false;
  }
  for (std::size_t i = 0; i < matrixNCells - nIn; ++i) {
    if (!setOp(i, cellNIn, cellIdxCellTID)) {
      return false;
    }
    addInsForCell(i, m, cellNIn, cellIdxCellTID);
  }

  return true;
}

auto MatrixGenerator::genM(std::unordered_set<std::size_t> &inputs,
                           std::vector<std::size_t> &outputs,
                           CellToNIn &cellNIn,
                           CellIdxToCellType &cellIdxCellTID) ->
  MatrixGenerator::Matrix {

  auto m(std::make_unique<std::vector<std::vector<char>>>(matrixNCells,
         std::vector<char>(matrixNCells, false)));

  setPrimIns(inputs);
  if (!setOps(m, cellNIn, cellIdxCellTID)) {
    return nullptr;
  }
  setPrimOuts(m, outputs, cellNIn, cellIdxCellTID);

  return m;
}

NetID MatrixGenerator::generateValid() {
  CellIdxToCellType cellIdxCellTID;
  std::unordered_set<std::size_t> inputs;
  std::vector<std::size_t> outputs;
  CellToNIn cellNIn;
  Matrix m = genM(inputs, outputs, cellNIn, cellIdxCellTID);

  if (m == nullptr) {
    return genInvalidNet();
  }

  NetBuilder netBuilder;

  std::unordered_map<std::size_t, CellID> mp;

  for (const auto &input : inputs) {
    auto cellID = makeCell(IN);
    mp[input] = cellID;
    netBuilder.addCell(cellID);
  }

  for (auto it = cellIdxCellTID.rbegin(); it != cellIdxCellTID.rend(); ++it) {
    std::size_t curCellN = it->first;
    Cell::LinkList inputs;
    for (long long j = (long long)(*m).size() - 1; j >= 0; --j) {
      if ((*m)[curCellN][j]) {
        inputs.push_back(LinkEnd((mp[j])));
      }
    }
    auto cellID = makeCell(it->second, inputs);
    mp[curCellN] = cellID;
    netBuilder.addCell(cellID);
  }

  for (const auto &output : outputs) {
    auto cellID = makeCell(OUT, mp[output]);
    netBuilder.addCell(cellID);
  }

  return netBuilder.make();
}

} // namespace eda::gate::model
