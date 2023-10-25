//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/unitized_table.h"

namespace eda::gate::optimizer2::resynthesis {

//===----------------------------------------------------------------------===//
// Constructors/Destructors
//===----------------------------------------------------------------------===//

UnitizedTable::UnitizedTable(const TruthTable &func, const TruthTable &care) {
  initialize(func, care);
}

//===----------------------------------------------------------------------===//
// Properties
//===----------------------------------------------------------------------===//

bool UnitizedTable::areInverse(unsigned c1, unsigned c2) const {
  unsigned id1 = columns[c1].first;
  unsigned id2 = columns[c2].first;

  if (((id1 == 62) && (id2 == 63)) || ((id1 == 63) && (id2 == 62))) {
    return true;
  }
  if ((id1 > 30) && (id1 < 62) && (id1 - 31 == id2)) {
    return true;
  }
  if ((id2 > 30) && (id2 < 62) && (id2 - 31 == id1)) {
    return true;
  }
  for (uint32_t i = 0; i < table.size(); i++) {
    if (getBit(i, c1) == getBit(i, c2)) {
      return false;
    }
  }
  return true;
}

//===----------------------------------------------------------------------===//
// Modification Methods
//===----------------------------------------------------------------------===//

void UnitizedTable::initialize(const TruthTable &func, const TruthTable &care) {
  uint32_t funcNumVars = func.num_vars();
  uint32_t careNumVars = care.num_vars();

  assert((funcNumVars <= varLimit) && (careNumVars <= varLimit) &&
         "Too many variables for Akers algorithm!");
  assert(funcNumVars == careNumVars &&
         "Number of variables of function and care are not equal!");
  columns.reserve(64);
  // create identifiers of columns
  // the identifiers for inputs from 0 to the number of variables
  for (unsigned i = 0; i < funcNumVars; i++) {
    columns.push_back(std::make_pair(i, 0));
  }
  // the identifiers for NOT(inputs) from 31 to (31 + the number of variables)
  for (unsigned i = 31; i < 31 + funcNumVars; i++) {
    columns.push_back(std::make_pair(i, 0));
  }
  // 62 will be for '0'
  // 63 will be for '1'
  columns.push_back(std::make_pair(62, 0));
  columns.push_back(std::make_pair(63, 0));
  
  uint32_t nCareSets = 0;
  // add rows
  for (uint32_t pos = 0; pos < func.num_bits(); pos++) {
    if (!kitty::get_bit(care, pos)) {
      nCareSets++;
      continue;
    }
    table.push_back(0);
    auto half = std::bitset<31>(pos);
    if (kitty::get_bit(func, pos)) {
      for (uint32_t j = 0; j < funcNumVars; j++) {
        if (half[j]) {
          setBit(pos - nCareSets, j);
          columns[j].second++;
        } else {
          setBit(pos - nCareSets, j + funcNumVars);
          columns[j + funcNumVars].second++;
        }
      }
      setBit(pos - nCareSets, 2 * funcNumVars + 1);
      columns[2 * funcNumVars + 1].second++;
    } else {
      for (uint32_t j = 0; j < funcNumVars; j++) {
        if (half[j]) {
          setBit(pos - nCareSets, j + funcNumVars);
          columns[j + funcNumVars].second++;
        } else {
          setBit(pos - nCareSets, j);
          columns[j].second++;
        }
      }
      setBit(pos - nCareSets, 2 * funcNumVars);
      columns[2 * funcNumVars].second++;
    }
  }
  assert(table.size() && "Empty input function!");

  reduce();
}

void UnitizedTable::addMajColumn(std::set<unsigned> args) {
  assert(columns.size() < 64 && "An overflow of the columns!");
  auto it = args.begin();
  unsigned c1 = *it++;
  unsigned c2 = *it++;
  unsigned c3 = *it;
  uint32_t nOnes = 0;

  for (uint32_t i = 0; i < table.size(); i++) {
    bool bit = (getBit(i, c1) && getBit(i, c2)) ||
               (getBit(i, c1) && getBit(i, c3)) ||
               (getBit(i, c2) && getBit(i, c3));
    if (bit) {
      setBit(i, columns.size());
      nOnes++;
    }
  }

  columns.push_back(std::make_pair(nMajGates + 64, nOnes));
  nMajGates++;
}

void UnitizedTable::eraseCol(unsigned index) {
  size_t columnsSize = columns.size();
  for (uint32_t i = 0; i < table.size(); i++) {
    if (index == columnsSize - 1) {
      clearBit(i, index);
    }
    for (unsigned j = index + 1; j < columnsSize; j++) {
      if (getBit(i, j)) {
        setBit(i, j - 1);
      } else {
        clearBit(i, j - 1);
      }
      clearBit(i, j);
    }
  }
  columns.erase(columns.begin() + index);
}

void UnitizedTable::reduce() {
  reduceRows();
  bool flag = reduceColumns();

  while (flag) {
    flag = reduceRows();
    if (!flag) {
      return;
    }
    flag = reduceColumns();
  }
}

//===----------------------------------------------------------------------===//
// Internal Methods
//===----------------------------------------------------------------------===//

void UnitizedTable::eraseCol(const std::vector<unsigned> &index) {
  if (index.empty()) {
    return;
  }
  if (index.size() == 1) {
    return eraseCol(index[0]);
  }

  size_t columnsSize = columns.size();
  size_t indexSize = index.size();
  for (uint32_t i = 0; i < table.size(); i++) {
    size_t pointVector = 1;
    unsigned pointPush = index[0];
    unsigned pointPop = index[0] + 1;
    if (pointPush == columnsSize - indexSize) {
      clearBit(i, pointPush);
    }
    for (; pointPop < columnsSize; pointPop++) {
      if (index[pointVector] == pointPop) {
        clearBit(i, pointPop);
        if (pointVector + 1 != indexSize) {
          pointVector++;
        }
        continue;
      }
      if (getBit(i, pointPop)) {
        setBit(i, pointPush);
      } else {
        clearBit(i, pointPush);
      }
      clearBit(i, pointPop);
      pointPush++;
    }
  }

  size_t i = indexSize;
  do {
    --i;
    columns.erase(columns.begin() + index[i]);
  } while (i);
}

bool UnitizedTable::isDegreeOfTwo(uint64_t row, unsigned &degree) const {
  if (!row) {
    assert(false && "Invalid condition of the table!");
  }
  if (!(row & (row - 1))) {
    degree = ffsll(row) - 1;
    return true;
  }
  return false;
}

void UnitizedTable::deleteRow(uint32_t row) {
  for (uint16_t i = 0; i < columns.size(); i++) {
    if (getBit(row, i)) {
      columns[i].second--;
    }
  }
}

bool UnitizedTable::reduceRows() {
  size_t columnsSize = columns.size();
  if ((columnsSize == 1) || (columnsSize == 3)) {
    return false;
  }
  std::set<uint32_t> rowsForRemoval;
  size_t tableSize = table.size();
  for (uint32_t i = 0; i < tableSize; i++) {
    for (uint32_t j = i + 1; j < tableSize; j++) {
      if (table[i] == table[j]) {
        rowsForRemoval.insert(i);
        break;
      }
      uint64_t res = table[i] | table[j];
      if (res == table[i]) {
        rowsForRemoval.insert(i);
        break;
      }
      if (res == table[j]) {
        rowsForRemoval.insert(j);
      }
    }
  }

  if (rowsForRemoval.empty()) {
    return false;
  }

  auto it = rowsForRemoval.end();
  do {
    --it;
    uint32_t rowNum = *it;
    deleteRow(rowNum);
    table.erase(table.begin() + rowNum);
  } while (it != rowsForRemoval.begin());

  return true;
}

bool UnitizedTable::reduceColumns() {
  size_t columnsSize = columns.size();
  if ((columnsSize == 1) || (columnsSize == 3)) {
    return false;
  }
  size_t tableSize = table.size();
  size_t selectedCols;
  std::unordered_set<unsigned> essentialCols;
  std::vector<unsigned> colsForRemoval;
  uint64_t mask = -1;
  unsigned degree;
  unsigned startPos = 0;
  bool mayRemove = true;
  bool wasRemoved = false;
  unsigned position = 0;

  std::vector<Column> cols;
  for (unsigned i = 0; i < columnsSize; i++) {
    cols.push_back(std::make_pair(i, columns[i].second));
  }
  std::sort(cols.begin(), cols.end(), [] (Column a, Column b) {
                                            return a.second < b.second;
                                          });

  for (unsigned i = 0; i < columnsSize - 1; i++) {
    for (uint32_t j = 0; j < tableSize; j++) {
      if (!mustCheck(wasRemoved, j, position)) {
        continue;
      }
      for (uint32_t k = j + 1; k < tableSize; k++) {
        if (!mustCheck(wasRemoved, k, position)) {
          continue;
        }
        if (isDegreeOfTwo(table[j] & table[k] & mask, degree)) {
          essentialCols.insert(degree);
          selectedCols = colsForRemoval.size() + essentialCols.size();
          if (selectedCols == columnsSize) {
            mayRemove = false;
            break;
          }
        }
      }
      if (!mayRemove) {
        break;
      }
    }
    if (!mayRemove) {
      break;
    }
    for (unsigned j = startPos; j < columnsSize; j++) {
      unsigned candidate = cols[j].first;
      if (essentialCols.find(candidate) == essentialCols.end()) {
        colsForRemoval.push_back(candidate);
        startPos = j + 1;
        wasRemoved = true;
        position = candidate;
        selectedCols = colsForRemoval.size() + essentialCols.size();
        if (selectedCols == columnsSize) {
          std::sort(colsForRemoval.begin(), colsForRemoval.end());
          eraseCol(colsForRemoval);
          return true;
        }
        mask &= ~(1ull << cols[j].first);
        break;
      }
    }
  }
  std::sort(colsForRemoval.begin(), colsForRemoval.end());
  eraseCol(colsForRemoval);
  return !colsForRemoval.empty();
}

bool UnitizedTable::mustCheck(bool wasReduced, uint32_t rowNum,
                                               unsigned removedColumn) {

  return !wasReduced || getBit(rowNum, removedColumn);
}

std::ostream& operator <<(std::ostream &out, const UnitizedTable &t) {
  // columns id
  size_t j = t.nColumns();
  do {
    --j;
    out << t.idColumn(j) << " ";
  } while (j);
  out << std::endl;
  // rows (sets of bits)
  for (uint32_t i = 0; i < t.nRows(); i++) {
    size_t j = t.nColumns();
    do {
      --j;
      out << t.getBit(i, j) << " ";
      unsigned id = t.idColumn(j);
      id /= 10;
      while (id) {
        out << " ";
        id /=10;
      }
    } while (j);
    out << std::endl;
  }
  return out;
}

} // namespace eda::gate::optimizer2::resynthesis
