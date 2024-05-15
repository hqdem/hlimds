//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"

#include <ctime>
#include <map>
#include <stdexcept>
#include <unordered_set>

namespace eda::gate::model {

using CellToNIn = std::unordered_map<std::size_t, uint16_t>;
using CellSymbolList = std::initializer_list<CellSymbol>;
using CellTypeIDList = std::initializer_list<CellTypeID>;

/**
 * @brief Base nets generator class.
 */
class Generator {
  const char* invalidCellTErrMsg = "Generator's base has invalid cell types.";

public:
  /**
   * @brief Set fanin upper bound lim for each cell.
   * If net base has irrelevant ops throws exception.
   */
  void setFaninHigh(const uint16_t faninHigh);
  /**
   * @brief Set fanin lower and upper bounds lim for each cell.
   * If net base has irrelevant ops throws exception.
   */
  void setFaninLim(const uint16_t faninLow, const uint16_t faninHigh);
  /// Set seed for generated net reproducibility.
  void setSeed(const unsigned seed);
  /// Allow generator to generate hierarchical nets.
  void setHierarchical(const bool hierarchical);
  /// Set upper bound of nesting depth.
  void setNestingMax(const std::size_t nestMax);

  /// Get current generator seed.
  unsigned getSeed() const;
  /// Get current generator name.
  virtual std::string getName() const = 0;

  /**
   * @brief Returns valid net id or invalid object id if it's impossible
   * to generate net using the parameters passed to the constructor.
   */
  NetID generate();

protected:
  Generator() = delete;

  /**
   * @brief Generator constructor.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined operation symbols only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  Generator(const std::size_t nIn,
            const std::size_t nOut,
            const std::vector<CellSymbol> &netBase,
            const unsigned seed);

  /**
   * @brief Generator constructor.
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined and custom operation identifiers only.
   * Inputs, outputs and constants are not allowed.
   * @param seed Seed for reproducibility of the result.
   */
  Generator(const std::size_t nIn,
            const std::size_t nOut,
            const std::vector<CellTypeID> &netBase,
            const unsigned seed);

  /**
   * @brief Returns valid net id or invalid object id if it is found during
   * generation, that net can't be generated.
   */
  virtual NetID generateValid() = 0;

  /**
   * @brief Checks if it is possible to add input for
   * considering cell according to the net basis.
   *
   * @param cellNIn The inputs number of considering cell.
   * @param nSourceCells The number of cells that can be
   * linked to considering one.
   */
  bool canAddIn(const uint16_t cellNIn, const std::size_t nSourceCells) const;

  /**
   * @brief Chooses operation from the generator basis relying to cell inputs
   * number and available to connect cells number.
   */
  CellTypeID chooseCellType(const uint16_t cellNIn,
                            const std::size_t nSourceCells);

  /// Generates invalid net.
  NetID genInvalidNet() const;

private:
  Generator(const std::size_t nIn, const std::size_t nOut, const unsigned seed);

  bool isOperation(const CellTypeID cellTID) const;

  bool primInsOutsNotEmpty() const;

  ///  Checks if passed val is inside [low, high]
  bool isBounded(const uint16_t val,
                 const uint16_t low,
                 const uint16_t high) const;

  bool canCreateNetCell(const uint16_t cellNIn) const;

  CellTypeID createNetCell();

protected:
  /// Number of PIs in the resulting net.
  std::size_t nIn;
  /// Number of POs in the resulting net.
  std::size_t nOut;
  /// Seed for reproducibility of the generated net.
  unsigned seed;
  /// Fanin number lower bound in the resulting net.
  uint16_t faninLow;
  /// Fanin number upper bound in the resulting net.
  uint16_t faninHigh;
  /// Allows generator to make hierarchical nets.
  bool hierarchical;
  /// Maximum nesting depth in current hierarchy level.
  std::size_t nestingDepth;
  /// Possible cell type indexes.
  std::vector<CellTypeID> netBase;

  /// Inputs number to possible cell type indexes (from net basis).
  std::map<uint16_t, std::vector<CellTypeID>> nInCellTIDs;

  /// Number of net cells (cells with net inside) in current net.
  unsigned int netCellsN;
};

} // namespace eda::gate::model
