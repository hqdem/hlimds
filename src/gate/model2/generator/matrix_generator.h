//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/generator/generator.h"

namespace eda::gate::model {

/**
 * @brief Generates net using matrix.
 */
class MatrixGenerator final : public Generator {
  using Matrix = std::unique_ptr<std::vector<std::vector<char>>>;
  using CellIdxToCellType = std::map<std::size_t, CellTypeID>;

public:
  MatrixGenerator() = delete;
  MatrixGenerator(const MatrixGenerator &other) = default;
  MatrixGenerator(MatrixGenerator &&other) = default;
  MatrixGenerator &operator=(const MatrixGenerator &other) = default;
  MatrixGenerator &operator=(MatrixGenerator &&other) = default;
  ~MatrixGenerator() = default;

  /**
   * @brief Matrix-based net generator constructor.
   * Generator is able to generate net only if it is possible to create
   * net with one primary output.
   * @param nCells Number of inner cells.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined operation symbols only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  MatrixGenerator(const std::size_t nCells, const std::size_t nIn,
                  const std::size_t nOut,
                  const std::vector<CellSymbol> &netBase,
                  const unsigned seed = 0u);

  /**
   * @brief Matrix-based net generator constructor.
   * Generator is able to generate net only if it is possible to create
   * net with one primary output.
   * @param nCells Number of inner cells.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined and custom operation identifiers only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  MatrixGenerator(const std::size_t nCells, const std::size_t nIn,
                  const std::size_t nOut,
                  const std::vector<CellTypeID> &netBase,
                  const unsigned seed = 0u);

  std::string getName() const override;

private:
  NetID generateValid() override;

  /// Sets primary inputs in matrix.
  void setPrimIns(std::unordered_set<std::size_t> &inputs);

  /// Checks if it is possible to make drain on column columnN.
  bool canMakeDrain(Matrix &m, const std::size_t columnN, CellToNIn &cellNIn);

  /// Sets primary outputs in matrix.
  void setPrimOuts(Matrix &m, std::vector<std::size_t> &outputs,
                   CellToNIn &cellNIn, CellIdxToCellType &cellIndCellTID);

  /// Sets "1" in each column in matrix (except the first one).
  bool setCellsOuts(Matrix &m, CellToNIn &cellNIn);
  void addInsForCell(const std::size_t rowN, Matrix &m, CellToNIn &cellNIn,
                     CellIdxToCellType &cellIndCellTID);
  bool setOp(const std::size_t i, CellToNIn &cellNIn,
             CellIdxToCellType &cellIndCellTID);

  /// Sets operations in matrix.
  bool setOps(Matrix &m, CellToNIn &cellNIn, CellIdxToCellType &cellIndCellTID);

  /// Generates adjacency matrix.
  Matrix genM(std::unordered_set<std::size_t> &inputs,
              std::vector<std::size_t> &outputs, CellToNIn &cellNIn,
              CellIdxToCellType &cellIndCellTID);

private:
  std::size_t matrixNCells;
};

} // namespace eda::gate::model
