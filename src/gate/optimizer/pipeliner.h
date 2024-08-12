//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <cmath>
#include <functional>
#include <set>
#include <stack>

static inline bool floatComp(const float &a, const float &b) {
  return b - a > 1e-6;
}

static inline bool delayCellComp(
    const std::pair<float, size_t> &a,
    const std::pair<float, size_t> &b) {

  if (std::fabs(a.first - b.first) < 1e-6) {
    return b.second > a.second;
  }
  return b.first - a.first > 1e-6;
}

namespace eda::gate::optimizer {

/// Finds next to optimal set of links to insert triggers into.
class Pipeliner final {
public:
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
  using LayerBounds = std::pair<size_t, size_t>;
  using CellLayerBounds = std::vector<LayerBounds>;
  using DelayCell = std::pair<float, size_t>;

  /**
   * @brief Pipelining algorithm state. Contains all the required intermediate
   * values.
   */
  struct PipeliningState final {
    PipeliningState() : delayCellSet(delayCellComp) {};

    std::vector<LayerBounds> layerBounds{};
    std::vector<std::vector<size_t>> fanouts{};
    std::vector<float> layerDelay{};
    float layerDelaySum{0.f};
    std::set<DelayCell, decltype(&delayCellComp)> delayCellSet;
  };

  /// The resulting markup of subnet links.
  struct SubnetMarkup final {
  public:
    SubnetMarkup() = default;
    SubnetMarkup(std::initializer_list<std::vector<size_t>> initList):
      markedLinks(initList) {};

    /// Returns number of triggers to add between entryID and its linkN'th link.
    size_t getTriggersN(const size_t entryID, const size_t linkN) const {
      if (markedLinks[entryID].size() <= linkN) {
        return 0;
      }
      return markedLinks[entryID][linkN];
    }

    std::vector<std::vector<size_t>> markedLinks{};
  };

  /**
   * @brief Constructs a pipeliner.
   *
   * @param k The required number of cascades in the resulting subnet.
   */
  Pipeliner(const size_t k): k(k) { assert(k >= 1); };

  /**
   * @brief Returns a structure with information about the number of triggers to
   * to be inserted into each link in the subnet.
   */
  SubnetMarkup markCascades(const SubnetBuilderPtr &builder) const;

private:
  /// Finds the provided subnet depth.
  size_t findSubnetDepth(const SubnetBuilderPtr &builder) const;

  /// Initializes pipelining algorithm state.

  /**
   * @brief Initializes pipelining algorithm state.
   * 1. Finds layer bounds for each entry.
   * 2. Finds fanouts for each entry.
   * 3. Finds delays for each entry.
   * 4. Finds preliminary delays for each layer.
   */
  void initPipeliningState(
      const SubnetBuilderPtr &builder,
      PipeliningState &state) const;

  /// Divides subnet cells into layers.
  void divideIntoLayers(
      const SubnetBuilderPtr &builder,
      PipeliningState &state) const;

  /// Updates the left layer bound of the provided cell.
  void updateLeftLayerBound(
      const SubnetBuilderPtr &builder,
      PipeliningState &state,
      const size_t entryID) const;

  /// Updates the right layer bound of the provided cell based on its parent.
  void updateRightLayerBound(
      const SubnetBuilderPtr &builder,
      PipeliningState &state,
      const size_t entryID,
      const size_t parEntryID,
      const size_t subnetDepth) const;

  /// Updates the layer delay based on the added cell.
  void updateLayerDelay(
      const SubnetBuilderPtr &builder,
      PipeliningState &state,
      const size_t layerN,
      const size_t entryID) const;

  /// Recursively limits entries layer bounds starting with i-th cell.

  /**
   * @brief Recursively limits the layer bounds of cells for the i-th cell and
   * its links and fanouts.
   * The algorithm adds cells in the layers of maximum delay that is acceptable
   * for the cell.
   */
  void limitLayerBounds(
      const SubnetBuilderPtr &builder,
      PipeliningState &state,
      const size_t entryID) const;

  /**
   * @brief Finds the layers with the maximum delay on the segment
   * [leftLayer; rightLayer].
   */
  size_t findMaxDelayLayer(
      const PipeliningState &state,
      const size_t leftLayer,
      const size_t rightLayer) const;

  /// Finds the entryID-th cell delay.
  float findDelay(
      const SubnetBuilderPtr &builder,
      const size_t entryID) const;

  /// Updates the parEntryID-th cell links fanouts.
  void updateLinksFanouts(
      const SubnetBuilderPtr &builder,
      PipeliningState &state,
      const size_t parEntryID) const;

  /**
   * @brief Marks the layers below which triggers should be inserted.
   * Finds the number of triggers below each cascade.
   */
  void markLayers(
      const PipeliningState &state,
      std::vector<size_t> &cascadePathTriggers,
      std::vector<size_t> &layerCascade) const;

  /// Marks links in the Subnet where triggers should be inserted.
  SubnetMarkup markLinks(
      const SubnetBuilderPtr &builder,
      const PipeliningState &state,
      const std::vector<size_t> &cascadePathTriggers,
      const std::vector<size_t> &layerCascade) const;

  /// Divides the subnet into cascades based on precomputed layers.
  SubnetMarkup divideIntoCascades(
      const SubnetBuilderPtr &builder,
      const PipeliningState &state) const;

private:
  const size_t k;

  static constexpr float floatEps = 1e-6;
};

} // namespace eda::gate::optimizer
