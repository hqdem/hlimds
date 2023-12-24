//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"

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
   * @brief Sets fanin upper bound lim for each cell.
   * If net base has irrelevant ops throws exception.
   */
  void setFaninHigh(const uint16_t faninHigh);

  /**
   * @brief Sets fanin lower and upper bounds lim for each cell.
   * If net base has irrelevant ops throws exception.
   */
  void setFaninLim(const uint16_t faninLow, const uint16_t faninHigh);
  void setSeed(const unsigned seed);
  void setHierarchical(const bool hierarchical);

  /**
   * @brief Sets upper bound of nesting depth.
   */
  void setNestingMax(const std::size_t nestMax);

  unsigned getSeed() const;
  virtual std::string getName() const = 0;

  /**
   * @brief Returns valid net id or invalid object id if it's impossible
   * to generate net using the parameters passed to the constructor.
   */
  NetID generate();

protected:
  Generator() = delete;
  Generator(const Generator &other) = default;
  Generator(Generator &&other) = default;
  Generator &operator=(const Generator &other) = default;
  Generator &operator=(Generator &&other) = default;
  virtual ~Generator() = default;

  Generator(const std::size_t nIn,
            const std::size_t nOut,
            const std::vector<CellSymbol> &netBase,
            const unsigned seed);

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
  std::size_t nIn, nOut;
  unsigned seed;
  uint16_t faninLow, faninHigh;
  bool hierarchical;
  std::size_t nestingDepth;
  std::vector<CellTypeID> netBase;

  std::map<uint16_t, std::vector<CellTypeID>> nInCellTIDs;

  /// Number of net cells (cells with net inside) in current net.
  unsigned int netCellsN;
};

} // namespace eda::gate::model
