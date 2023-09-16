//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <set>
#include <string>
#include <strings.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kitty/bit_operations.hpp"
#include "kitty/dynamic_truth_table.hpp"

namespace eda::gate::optimizer2::resynthesis {
  class AkersAlgorithm;
} // namespace eda::gate::optimizer2::resynthesis

namespace eda::gate::optimizer2::resynthesis {

constexpr unsigned varLimit = 31;

/**
 * \brief Implements a unitized truth table for Akers algorithm.
*/
class UnitizedTable {
  friend class AkersAlgorithm;

public:

  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//

  using Column = std::pair<unsigned, uint32_t>;
  using Columns = std::vector<Column>;
  using TruthTable = kitty::dynamic_truth_table;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Empty constructor.
  UnitizedTable() {};

  /// Constructs unitized table of "func" without the sets where "care" has 0.
  UnitizedTable(const TruthTable &func, const TruthTable &care);
  
  //===--------------------------------------------------------------------===//
  // Properties
  //===--------------------------------------------------------------------===//

  /// Checks whether two columns are inverse one relatively to another.
  bool areInverse(unsigned c1, unsigned c2) const;

  //===--------------------------------------------------------------------===//
  // Statistics
  //===--------------------------------------------------------------------===//
  
  /// Returns bit in the particular place in the table.
  bool getBit(uint32_t rowNum, unsigned index) const {
    return (table[rowNum] >> index) & 1;
  }

  /// Returns the row.
  uint64_t getRow(uint32_t rowNum) const {
    return table[rowNum];
  }

  /// Returns the number of columns.
  uint64_t nColumns() const {
    return columns.size();
  }
  
  /// Returns the number of rows.
  uint64_t nRows() const {
    return table.size();
  }

  /// Returns the column id.
  unsigned idColumn(unsigned index) const {
    return columns[index].first;
  }
  
  //===--------------------------------------------------------------------===//
  // Modification Methods
  //===--------------------------------------------------------------------===//

  /// Initializer function.
  void initialize(const TruthTable &func, const TruthTable &care);
  
  /// Sets the bit in the table.
  void setBit(uint32_t rowNum, unsigned index) {
    table[rowNum] |= (1ull << index);
  }

  /// Zeros the bit in the table.
  void clearBit(uint32_t rowNum, unsigned index) {
    table[rowNum] &= ~(1ull << index);
  }

  /// Adds the column that corresponds with an output of a majority gate.
  void addMajColumn(std::set<unsigned> args);

  /// Erases one column.
  void eraseCol(unsigned index);

  /// Reduces the number of rows and columns if it possible.
  void reduce();

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//
  
  /// Erases columns in the table.
  void eraseCol(const std::vector<unsigned> &index);

  /// Checks whether row has only one bit.
  bool isDegreeOfTwo(uint64_t row, unsigned &degree) const;

  /// Updates the number of ones in columns after a deletion 'row'.
  void deleteRow(uint32_t row);

  /// Reduces the number of rows if it possible.
  bool reduceRows();

  /// Reduces the number of columns if it possible.
  bool reduceColumns();

  /// Shows whether we must check if ones are "essential".
  bool mustCheck(bool wasReduced, uint32_t rowNum, unsigned removedColumn);

  //===--------------------------------------------------------------------===//
  // Internal Fields
  //===--------------------------------------------------------------------===//

  /// Unitized table, one object of the vector is one row with set of bits.
  std::vector<uint64_t> table;

  /// Identifiers of columns.
  Columns columns;

  /// Number of majority functions that were added to the table.
  uint32_t nMajGates = 0;
};

/// Outputs the table
std::ostream& operator <<(std::ostream &out, const UnitizedTable &t);

} // namespace eda::gate::optimizer2::resynthesis
