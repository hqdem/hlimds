//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/generator/generator.h"

namespace eda::gate::model {

/**
 * @brief Generates net using matrix.
 */
class MatrixGenerator final : public Generator {
  using Matrix = std::unique_ptr<std::vector<std::vector<char>>>;
  using CellIdxToCellType = std::map<std::size_t, CellTypeID>;

public:
  MatrixGenerator() = delete;

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const unsigned)
   * Generates net with matrix. Generator is able to generate net only if it is
   * possible to create net with one primary output.
   * @param nCells Number of inner cells.
   */
  MatrixGenerator(const std::size_t nCells,
                  const std::size_t nIn,
                  const std::size_t nOut,
                  const std::vector<CellSymbol> &netBase,
                  const unsigned seed = 0u);

  /**
   * @copydoc MatrixGenerator(const std::size_t, const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const unsigned)
   */
  MatrixGenerator(const std::size_t nCells,
                  const std::size_t nIn,
                  const std::size_t nOut,
                  const CellSymbolList &netBase,
                  const unsigned seed = 0u);

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const unsigned)
   * Generates net with matrix. Generator is able to generate net only if it is
   * possible to create net with one primary output.
   * @param nCells Number of inner cells.
   */
  MatrixGenerator(const std::size_t nCells,
                  const std::size_t nIn,
                  const std::size_t nOut,
                  const std::vector<CellTypeID> &netBase,
                  const unsigned seed = 0u);

  /**
   * @copydoc MatrixGenerator(const std::size_t, const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const unsigned)
   */
  MatrixGenerator(const std::size_t nCells,
                  const std::size_t nIn,
                  const std::size_t nOut,
                  const CellTypeIDList &netBase,
                  const unsigned seed = 0u);

  /// Get matrix generator name.
  std::string getName() const override;

private:
  NetID generateValid() override;

  /// Sets primary inputs in matrix.
  void setPrimIns(std::unordered_set<std::size_t> &inputs);

  /// Checks if it is possible to make drain on column columnN.
  bool canMakeDrain(Matrix &m, const std::size_t columnN, CellToNIn &cellNIn);

  /// Sets primary outputs in matrix.
  void setPrimOuts(Matrix &m,
                   std::vector<std::size_t> &outputs,
                   CellToNIn &cellNIn,
                   CellIdxToCellType &cellIndCellTID);

  /// Sets "1" in each column in matrix (except the first one).
  bool setCellsOuts(Matrix &m, CellToNIn &cellNIn);

  void addInsForCell(const std::size_t rowN,
                     Matrix &m,
                     CellToNIn &cellNIn,
                     CellIdxToCellType &cellIndCellTID);

  bool setOp(const std::size_t i,
             CellToNIn &cellNIn,
             CellIdxToCellType &cellIndCellTID);

  /// Sets operations in matrix.
  bool setOps(Matrix &m, CellToNIn &cellNIn, CellIdxToCellType &cellIndCellTID);

  /// Generates adjacency matrix.
  Matrix genM(std::unordered_set<std::size_t> &inputs,
              std::vector<std::size_t> &outputs,
              CellToNIn &cellNIn,
              CellIdxToCellType &cellIndCellTID);

private:
  std::size_t matrixNCells;
};

} // namespace eda::gate::model
