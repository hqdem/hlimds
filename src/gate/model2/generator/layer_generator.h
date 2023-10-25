//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/generator/generator.h"

#include <random>

namespace eda::gate::model {

/**
 * @brief Generates layered net.
 */
class LayerGenerator final : public Generator {

  struct CellIDHash {
    std::uint64_t operator()(const CellID &cellID) const {
      return std::hash<std::uint64_t>{}(cellID.getFID());
    }
  };

public:
  LayerGenerator() = delete;
  LayerGenerator(const LayerGenerator &other) = default;
  LayerGenerator(LayerGenerator &&other) = default;
  LayerGenerator &operator=(const LayerGenerator &other) = default;
  LayerGenerator &operator=(LayerGenerator &&other) = default;
  ~LayerGenerator() = default;

  /**
   * @brief Layered net generator constructor.
   *
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined operation symbols only.
   * Inputs, outputs and constants are not allowed.
   * @param layerNCells Number of cells on each layer except the first one and
   * primary outputs layer. The first layer contains only primary inputs.
   * @param seed Seed for reproducibility of the result.
   */
  LayerGenerator(const int nIn, const int nOut,
                 const std::vector<CellSymbol> &netBase,
                 const std::vector<int> &layerNCells, const unsigned seed = 0u);

  /**
   * @brief Layered net generator constructor.
   *
   * @param nIn Number of primary inputs.
   * @param nOut Number of primary outputs.
   * @param netBase Basis of allowed operations.
   * Basis can contain predefined and custom operation identifiers only.
   * Inputs, outputs and constants are not allowed.
   * @param layerNCells Number of cells on each layer except the first one and
   * primary outputs layer. The first layer contains only primary inputs.
   * @param seed Seed for reproducibility of the result.
   */
  LayerGenerator(const int nIn, const int nOut,
                 const std::vector<CellTypeID> &netBase,
                 const std::vector<int> &layerNCells, const unsigned seed = 0u);

  std::string getName() const override;

private:
  /**
   * @brief Generates net if it is possible to connect every layer with next
   * layer or primary output layer.
   */
  NetID generateValid() override;

  /// Sets primary inputs in netBuilder.
  void setPrimIns(NetBuilder &netBuilder, std::vector<CellID> &prevLayerCells,
                  std::vector<CellID> &addedCells);

  /// Sets primary outputs in netBuilder.
  bool setPrimOuts(NetBuilder &netBuilder, std::vector<CellID> &prevLayerCells,
                   std::vector<CellID> &addedCells,
                   std::vector<CellID> &outputs);

  /// Creating new layer in netBuilder.
  bool setLayerCells(NetBuilder &netBuilder,
                     std::vector<Cell::LinkList> &curLayerIns,
                     std::vector<CellID> &prevLayerCells,
                     std::vector<CellID> &addedCells);

  /// Links the previous layer with the current one in netBuilder.
  bool linkPrevLayer(const int cellOnLayer,
                     std::vector<Cell::LinkList> &curLayerIns,
                     std::vector<CellID> &prevLayerCells,
                     std::vector<CellID> &addedCells,
                     std::vector<CellID> &outputs);

  /// Sets inputs for cell.
  void setInputs(Cell::LinkList &curInputs, const CellTypeID cellTID,
                 std::vector<CellID> &addedCells);

  /// Sets operation for cell in netBuilder.
  bool setOp(std::vector<CellID> &curLayerCells, Cell::LinkList &curCellIns,
             std::vector<CellID> &addedCells, NetBuilder &netBuilder);

private:
  std::vector<int> layerNCells;
};

} // namespace eda::gate::model
