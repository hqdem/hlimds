//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/generator/matrix_generator.h"

namespace eda::gate::model {

MatrixGenerator::MatrixGenerator(const int nCells, const int nIn,
                                 const int nOut,
                                 const std::vector<CellSymbol> &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

MatrixGenerator::MatrixGenerator(const int nCells, const int nIn,
                                 const int nOut,
                                 const std::vector<CellTypeID> &netBase,
                                 const unsigned seed):
  Generator(nIn, nOut, netBase, seed), matrixNCells(nCells + nIn) {};

std::string MatrixGenerator::getName() const {
  return "MatrixGenerator";
}

void MatrixGenerator::setPrimIns(std::unordered_set<int> &inputs) {
  while ((int)inputs.size() != nIn) {
    inputs.insert(matrixNCells - inputs.size() - 1);
  }
}

bool MatrixGenerator::canMakeDrain(Matrix &m, const int columnN,
                                   CellToNIn &cellNIn) {

  for (int i = 0; i < matrixNCells; ++i) {
    const bool curRowHasIn = (*m)[i][columnN];
    const bool noLessInNOps = nInCellTIDs.find(cellNIn[i] - 1) ==
                              nInCellTIDs.end();
    const bool noAnyNInOps = nInCellTIDs.find(CellType::AnyArity) == nInCellTIDs.end();
    if (curRowHasIn && (noLessInNOps && (cellNIn[i] < 3 || noAnyNInOps))) {
      return false;
    }
  }
  return true;
}

void MatrixGenerator::setPrimOuts(Matrix &m, std::vector<int> &outputs,
                                  CellToNIn &cellNIn,
                                  std::map<int, CellTypeID> &cellIndCellTID) {

  outputs.push_back(0);
  for (int k = 0; k < nOut - 1; ++k) {
    if (!(std::rand() % 2)) {
      continue;
    }
    assert(matrixNCells != 0);
    int j = std::rand() % matrixNCells;

    if (!canMakeDrain(m, j, cellNIn)) {
      continue;
    }
    for (int i = 0; i < matrixNCells; ++i) {
      if ((*m)[i][j]) {
        if (nInCellTIDs.find(cellNIn[i] - 1) != nInCellTIDs.end()) {
          int opN = std::rand() % nInCellTIDs[cellNIn[i] - 1].size();
          cellIndCellTID[i] = nInCellTIDs[cellNIn[i] - 1][opN];
        } else {
          int opN = std::rand() % nInCellTIDs[CellType::AnyArity].size();
          cellIndCellTID[i] = nInCellTIDs[CellType::AnyArity][opN];
        }
        cellNIn[i]--;
      }
      (*m)[i][j] = false;
    }
    outputs.push_back(j);
  }

  while (nOut > (int)outputs.size()) {
    assert(matrixNCells != 0);
    outputs.push_back(std::rand() % matrixNCells);
  }
}

bool MatrixGenerator::setCellsOuts(Matrix &m, CellToNIn &cellNIn) {
  for (int j = 1; j < matrixNCells; ++j) {
    int availRowsN = std::min(j, matrixNCells - nIn);
    std::unordered_set<int> unavailRows;
    assert(availRowsN != 0);
    int i;
    do {
      i = std::rand() % availRowsN;
      if (!canAddIn(cellNIn[i], matrixNCells - i - 1)) {
        unavailRows.insert(i);
      } else {
        break;
      }
    } while ((int)unavailRows.size() != availRowsN);
    if ((int)unavailRows.size() == availRowsN) {
      return false;
    }

    (*m)[i][j] = true;
    cellNIn[i]++;
  }

  return true;
}

void MatrixGenerator::addInsForCell(const int rowN, Matrix &m,
                                    CellToNIn &cellNIn,
                                    std::map<int, CellTypeID> &cellIndCellTID) {

  int neededCurNIn;
  const CellType *cellT = &CellType::get(cellIndCellTID[rowN]);
  if (cellT->isAnyArity()) {
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
    int j = std::rand() % (matrixNCells - rowN - 1) + rowN + 1;
    if (!(*m)[rowN][j]) {
      (*m)[rowN][j] = true;
      cellNIn[rowN] += (*m)[rowN][j] ? 1 : 0;
    }
  }
}

bool MatrixGenerator::setOp(const int i, CellToNIn &cellNIn,
                            std::map<int, CellTypeID> &cellIndCellTID) {

  CellTypeID cellTID = chooseCellType(cellNIn[i], matrixNCells - i - 1);
  if (cellTID == OBJ_NULL_ID) {
    return false;
  }
  cellIndCellTID[i] = cellTID;

  return true;
}

bool MatrixGenerator::setOps(Matrix &m, CellToNIn &cellNIn,
                             std::map<int, CellTypeID> &cellIndCellTID) {

  if (!setCellsOuts(m, cellNIn)) {
    return false;
  }
  for (int i = 0; i < matrixNCells - nIn; ++i) {
    if (!setOp(i, cellNIn, cellIndCellTID)) {
      return false;
    }
    addInsForCell(i, m, cellNIn, cellIndCellTID);
  }

  return true;
}

auto MatrixGenerator::genM(std::unordered_set<int> &inputs,
                           std::vector<int> &outputs, CellToNIn &cellNIn,
                           std::map<int, CellTypeID> &cellIndCellTID) ->
  MatrixGenerator::Matrix {

  auto m(std::make_unique<std::vector<std::vector<char>>>(matrixNCells,
         std::vector<char>(matrixNCells, false)));

  setPrimIns(inputs);
  if (!setOps(m, cellNIn, cellIndCellTID)) {
    return nullptr;
  }
  setPrimOuts(m, outputs, cellNIn, cellIndCellTID);

  return m;
}

NetID MatrixGenerator::generateValid() {
  std::map<int, CellTypeID> cellIndCellTID;
  std::unordered_set<int> inputs;
  std::vector<int> outputs;
  CellToNIn cellNIn;
  Matrix m = genM(inputs, outputs, cellNIn, cellIndCellTID);

  if (m == nullptr) {
    return genEmptyNet();
  }

  NetBuilder netBuilder;

  std::unordered_map<int, CellID> mp;

  for (const auto &input : inputs) {
    auto cellID = makeCell(IN);
    mp[input] = cellID;
    netBuilder.addCell(cellID);
  }

  for (auto it = cellIndCellTID.rbegin(); it != cellIndCellTID.rend(); ++it) {
    int curCellN = it->first;
    Cell::LinkList inputs;
    for (int j = (*m).size() - 1; j >= 0; --j) {
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
