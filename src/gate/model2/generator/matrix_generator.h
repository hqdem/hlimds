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

public:
  MatrixGenerator() = delete;
  MatrixGenerator(const MatrixGenerator &other) = default;
  MatrixGenerator(MatrixGenerator &&other) = default;
  MatrixGenerator &operator=(const MatrixGenerator &other) = default;
  MatrixGenerator &operator=(MatrixGenerator &&other) = default;
  ~MatrixGenerator() = default;

  /**
   * @brief Matrix-based net generator constructor.
   *
   * @param nCells Number of inner cells.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined operation symbols only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  MatrixGenerator(const int nCells, const int nIn, const int nOut,
                  const std::vector<CellSymbol> &netBase,
                  const unsigned seed = 0u);

  /**
   * @brief Matrix-based net generator constructor.
   *
   * @param nCells Number of inner cells.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined and custom operation identifiers only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  MatrixGenerator(const int nCells, const int nIn, const int nOut,
                  const std::vector<CellTypeID> &netBase,
                  const unsigned seed = 0u);

  std::string getName() const override;

private:
  /**
   * @brief Generates net if it is possible to generateValid connected
   *        net with one primary output.
   */
  NetID generateValid() override;

  /// Sets primary inputs in matrix.
  void setPrimIns(std::unordered_set<int> &inputs);

  /// Checks if it is possible to make drain on column columnN.
  bool canMakeDrain(Matrix &m, const int columnN, CellToNIn &cellNIn);

  /// Sets primary outputs in matrix.
  void setPrimOuts(Matrix &m, std::vector<int> &outputs, CellToNIn &cellNIn,
                   std::map<int, CellTypeID> &cellIndCellTID);

  /// Sets "1" in each column in matrix (except the first one).
  bool setCellsOuts(Matrix &m, CellToNIn &cellNIn);
  void addInsForCell(const int rowN, Matrix &m, CellToNIn &cellNIn,
                     std::map<int, CellTypeID> &cellIndCellTID);
  bool setOp(const int i, CellToNIn &cellNIn,
             std::map<int, CellTypeID> &cellIndCellTID);

  /// Sets operations in matrix.
  bool setOps(Matrix &m, CellToNIn &cellNIn,
              std::map<int, CellTypeID> &cellIndCellTID);

  /// Generates adjacency matrix.
  Matrix genM(std::unordered_set<int> &inputs, std::vector<int> &outputs,
              CellToNIn &cellNIn, std::map<int, CellTypeID> &cellIndCellTID);

private:
  int matrixNCells;
};

} // namespace eda::gate::model
