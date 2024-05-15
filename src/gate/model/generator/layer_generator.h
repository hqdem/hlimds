//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/generator/generator.h"

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

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const unsigned)
   * Generates net layer by layer. Generator is able to generate net only if it
   * is possible to connect every element on a layer with the next layer or
   * primary output layer.
   * @param layerNCells Number of cells on each layer except the first one and
   * primary outputs layer. The first layer contains only primary inputs.
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const std::vector<CellSymbol> &netBase,
                 const std::vector<std::size_t> &layerNCells,
                 const unsigned seed = 0u);

  /**
   * @copydoc LayerGenerator(const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const std::vector<std::size_t> &, const unsigned)
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const CellSymbolList &netBase,
                 const std::vector<std::size_t> &layerNCells,
                 const unsigned seed = 0u);

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const unsigned)
   * Generates net layer by layer. Generator is able to generate net only if it
   * is possible to connect every element on a layer with the next layer.
   * @param nLayers Number of layers. It should be possible with the passed
   * net basis to reduce number of cells on layer from nIn to nOut with nLayers.
   * @param layerNCellsMin Minimum number of cells on each layer. Should be less
   * or equal to nOut.
   * @param layerNCellsMax Maximum number of cells on each layer. Second layer
   * should be connectable with first layer (with primary inputs).
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const std::vector<CellSymbol> &netBase,
                 const std::size_t nLayers,
                 const uint16_t layerNCellsMin,
                 const uint16_t layerNCellsMax,
                 const unsigned seed = 0u);

  /**
   * @copydoc LayerGenerator(const std::size_t, const std::size_t, const std::vector<CellSymbol> &, const std::size_t, const uint16_t, const uint16_t, const unsigned)
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const CellSymbolList &netBase,
                 const std::size_t nLayers,
                 const uint16_t layerNCellsMin,
                 const uint16_t layerNCellsMax,
                 const unsigned seed = 0u);

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const unsigned)
   * Generates net layer by layer. Generator is able to generate net only if it
   * is possible to connect every element on a layer with the next layer or
   * primary output layer.
   * @param layerNCells Number of cells on each layer except the first one and
   * primary outputs layer. The first layer contains only primary inputs.
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const std::vector<CellTypeID> &netBase,
                 const std::vector<std::size_t> &layerNCells,
                 const unsigned seed = 0u);

  /**
   * @copydoc LayerGenerator(const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const std::vector<std::size_t> &, const unsigned)
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const CellTypeIDList &netBase,
                 const std::vector<std::size_t> &layerNCells,
                 const unsigned seed = 0u);

  /**
   * @copydoc Generator::Generator(const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const unsigned)
   * Generates net layer by layer. Generator is able to generate net only if it
   * is possible to connect every element on a layer with the next layer.
   * @param nLayers Number of layers. It should be possible with the passed
   * net basis to reduce number of cells on layer from nIn to nOut with nLayers.
   * @param layerNCellsMin Minimum number of cells on each layer. Should be less
   * or equal to nOut.
   * @param layerNCellsMax Maximum number of cells on each layer. Second layer
   * should be connectable with first layer (with primary inputs).
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const std::vector<CellTypeID> &netBase,
                 const std::size_t nLayers,
                 const uint16_t layerNCellsMin,
                 const uint16_t layerNCellsMax,
                 const unsigned seed = 0u);

  /**
   * @copydoc LayerGenerator(const std::size_t, const std::size_t, const std::vector<CellTypeID> &, const std::size_t, const uint16_t, const uint16_t, const unsigned)
   */
  LayerGenerator(const std::size_t nIn,
                 const std::size_t nOut,
                 const CellTypeIDList &netBase,
                 const std::size_t nLayers,
                 const uint16_t layerNCellsMin,
                 const uint16_t layerNCellsMax,
                 const unsigned seed = 0u);

  /// Get layer generator name.
  std::string getName() const override;

private:
  NetID generateValid() override;

  /// Sets primary inputs in netBuilder.
  void setPrimIns(NetBuilder &netBuilder,
                  std::vector<CellID> &prevLayerCells,
                  std::vector<CellID> &addedCells);

  /// Sets primary outputs in netBuilder.
  bool setPrimOuts(NetBuilder &netBuilder,
                   std::vector<CellID> &prevLayerCells,
                   std::vector<CellID> &addedCells,
                   std::vector<CellID> &outputs);

  /// Creating new layer in netBuilder.
  bool setLayerCells(NetBuilder &netBuilder,
                     std::vector<Cell::LinkList> &curLayerIns,
                     std::vector<CellID> &prevLayerCells,
                     std::vector<CellID> &addedCells);

  /// Links the previous layer with the current one in netBuilder.
  bool linkPrevLayer(const std::size_t cellOnLayer,
                     std::vector<Cell::LinkList> &curLayerIns,
                     std::vector<CellID> &prevLayerCells,
                     std::vector<CellID> &addedCells,
                     std::vector<CellID> &outputs);

  /// Sets inputs for cell.
  void setInputs(Cell::LinkList &curInputs,
                 const CellTypeID cellTID,
                 std::vector<CellID> &addedCells);

  /// Sets operation for cell in netBuilder.
  bool setOp(std::vector<CellID> &curLayerCells,
             Cell::LinkList &curCellIns,
             std::vector<CellID> &addedCells,
             NetBuilder &netBuilder);

  bool generateLayerNCells(const std::size_t nLayers,
                           uint16_t layerNCellsMin,
                           uint16_t layerNCellsMax);

private:
  std::vector<std::size_t> layerNCells;

  std::size_t nLayers {0};
  std::size_t layerNCellsMin {0};
  std::size_t layerNCellsMax {0};
};

} // namespace eda::gate::model
