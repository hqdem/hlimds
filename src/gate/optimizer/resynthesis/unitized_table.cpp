#include "gate/optimizer/resynthesis/unitized_table.h"

namespace eda::gate::optimizer::resynthesis {

//===----------------------------------------------------------------------===//
// Constructors/Destructors
//===----------------------------------------------------------------------===//

UnitizedTable::UnitizedTable(const TruthTable &func, const TruthTable &care):
                             nMajGates(0) {

  uint32_t funcNumVars = func.num_vars();
  uint32_t careNumVars = care.num_vars();

  assert((funcNumVars <= varLimit) && (careNumVars <= varLimit) &&
         "Too many variables for Akers algorithm!");
  assert(funcNumVars == careNumVars &&
         "Number of variables of function and care are not equal!");

  std::vector<uint32_t> ones;
  ones.assign(2 * funcNumVars + 2, 0);
  // create identifiers of columns
  // the identifiers for inputs from 0 to the number of variables
  for (uint32_t i = 0; i < funcNumVars; i++) {
    columns.push_back(i);
  }
  // the identifiers for NOT(inputs) from 31 to (31 + the number of variables)
  for (uint32_t i = 31; i < 31 + funcNumVars; i++) {
    columns.push_back(i);
  }
  // 62 will be for '0'
  // 63 will be for '1'
  columns.push_back(62);
  columns.push_back(63);
  uint32_t dontCareCount = 0;

  // add rows
  for (size_t pos = 0; pos < func.num_bits(); pos++) {
    if (!kitty::get_bit(care, pos)) {
      dontCareCount++;
      continue;
    }
    table.push_back(0);
    auto half = std::bitset<31>(pos);
    if (kitty::get_bit(func, pos)) {
      for (uint32_t j = 0; j < funcNumVars; j++) {
        if (half[j]) {
          setBit(pos - dontCareCount, j);
          ones[j]++;
        } else {
          setBit(pos - dontCareCount, j + funcNumVars);
          ones[j + funcNumVars]++;
        }
      }
      setBit(pos - dontCareCount, 2 * funcNumVars + 1);
      ones[2 * funcNumVars + 1]++;
    } else {
      for (uint32_t j = 0; j < funcNumVars; j++) {
        if (half[j]) {
          setBit(pos - dontCareCount, j + funcNumVars);
          ones[j + funcNumVars]++;
        } else {
          setBit(pos - dontCareCount, j);
          ones[j]++;
        }
      }
      setBit(pos - dontCareCount, 2 * funcNumVars);
      ones[2 * funcNumVars]++;
    }
  }
  assert(table.size() && "Empty input function!");

  uint64_t maxOnes = table.size() / 2;
  unsigned saveColumn = 64;
  for (uint32_t i = 0; i < ones.size(); i++) {
    if (ones[i] > maxOnes) {
      saveColumn = i;
      maxOnes = ones[i];
    }
  }
  if (maxOnes == table.size()) {
    std::vector<unsigned> forRemovalCols;
    for (unsigned i = 0; i < columns.size(); i++) {
      if (i == saveColumn) {
        continue;
      }
      forRemovalCols.push_back(i);
    }
    eraseCol(forRemovalCols);
  } else {
    reduceColumns(saveColumn);
  }

  reduce();
}

//===----------------------------------------------------------------------===//
// Properties
//===----------------------------------------------------------------------===//

bool UnitizedTable::areInverse(unsigned c1, unsigned c2) const {
  unsigned id1 = columns[c1];
  unsigned id2 = columns[c2];

  if ((id1 > 30) && (id1 - 31 == id2)) {
    return true;
  }
  if ((id2 > 30) && (id2 - 31 == id1)) {
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
// Statistics
//===----------------------------------------------------------------------===//

uint64_t UnitizedTable::countEssentialOnes() const {
  size_t tableSize = table.size();
  unsigned degree;

  // unsigned is a number of column with essential one
  // std::set is a set of numbers of rows with essential one
  std::unordered_map<unsigned, std::unordered_set<uint32_t>> essentialOnes;
  uint64_t counter = 0;
  for (uint32_t i = 0; i < tableSize; i++) {
    for (uint32_t j = i + 1; j < tableSize; j++) {
      if (isDegreeOfTwo(table[i] & table[j], degree)) {
        if (essentialOnes.find(degree) == essentialOnes.end()) {
          essentialOnes[degree].insert(i);
          essentialOnes[degree].insert(j);
          counter += 2;
        } else {
          auto p = essentialOnes[degree].insert(i);
          if (p.second) {
            counter++;
          }
          p = essentialOnes[degree].insert(j);
          if (p.second) {
            counter++;
          }
        }
      }
    }
  }
  return counter;
}

//===----------------------------------------------------------------------===//
// Modification Methods
//===----------------------------------------------------------------------===//

void UnitizedTable::addMajColumn(std::set<unsigned> args) {
  assert(columns.size() < 64 && "An overflow of the columns!");

  auto it = args.begin();
  unsigned c1 = *it++;
  unsigned c2 = *it++;
  unsigned c3 = *it;

  for (uint32_t i = 0; i < table.size(); i++) {
    bool bit = (getBit(i, c1) && getBit(i, c2)) ||
               (getBit(i, c1) && getBit(i, c3)) ||
               (getBit(i, c2) && getBit(i, c3));
    if (bit) {
      setBit(i, columns.size());
    }
  }

  columns.push_back(nMajGates + 64);
  nMajGates++;
}

void UnitizedTable::eraseCol(unsigned index) {
  size_t columnsSize = columns.size();
  for (uint32_t i = 0; i < table.size(); i++) {
    if (index == columnsSize - 1) {
      table[i] &= ~(1ull << index);
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
  for (uint32_t i = 0; i < table.size(); i++) {
    size_t pointVector = 1;
    unsigned pointPush = index[0];
    unsigned pointPop = index[0] + 1;
    if (pointPush == columnsSize - 2) {
      clearBit(i, pointPush);
    }
    for (; pointPop < columnsSize; pointPop++) {
      if (index[pointVector] == pointPop) {
        clearBit(i, pointPop);
        if (pointVector + 1 != index.size()) {
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

  size_t i = index.size();
  do {
    --i;
    columns.erase(columns.begin() + index[i]);
  } while (i);
}

bool UnitizedTable::isDegreeOfTwo(uint64_t row, unsigned &degree) const {
  bool flag = false;
  for (unsigned i = 0; i < columns.size(); i++) {
    if ((row >> i) & 1) {
      if (flag) {
        return false;
      }
      degree = i;
      flag = true;
    }
  }
  return flag;
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
    table.erase(table.begin() + (*it));
  } while (it != rowsForRemoval.begin());

  return true;
}

bool UnitizedTable::reduceColumns(unsigned saveColumn) {
  size_t columnsSize = columns.size();
  if ((columnsSize == 1) || (columnsSize == 3)) {
    return false;
  }

  unsigned isSave = 0;
  if (saveColumn < columnsSize) {
    isSave = 1;
  }

  size_t tableSize = table.size();
  size_t selectedCols;
  std::unordered_set<unsigned> essentialCols;
  std::vector<unsigned> colsForRemoval;
  uint64_t mask = -1;
  unsigned degree;
  unsigned startPos = 0;
  bool mayRemove = true;

  for (unsigned i = 0; i < columnsSize - 1; i++) {
    for (uint32_t j = 0; j < tableSize; j++) {
      for (uint32_t k = j + 1; k < tableSize; k++) {
        if (isDegreeOfTwo(table[j] & table[k] & mask, degree)) {
          essentialCols.insert(degree);
          selectedCols = colsForRemoval.size() + essentialCols.size() + isSave;
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
      if (j == saveColumn) {
        continue;
      }
      if (essentialCols.find(j) == essentialCols.end()) {
        colsForRemoval.push_back(j);
        startPos = j + 1;
        selectedCols = colsForRemoval.size() + essentialCols.size() + isSave;
        if (selectedCols == columnsSize) {
          eraseCol(colsForRemoval);
          return true;
        }
        mask &= ~(1ull << j);
        break;
      }
    }
  }
  eraseCol(colsForRemoval);
  return !colsForRemoval.empty();
}

std::ostream& operator <<(std::ostream &out, const UnitizedTable &t) {
  // columns id
  size_t j = t.nColumns();
  do {
    --j;
    out << t.idColumn(j) << " ";
  } while (j);
  out << "\n";
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
    out << "\n";
  }
  return out;
}

}; // namespace eda::gate::optimizer::resynthesis
