//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/iomapping.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/cut_extractor.h"
#include "util/truth_table.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet View
//===----------------------------------------------------------------------===//

/**
 * @brief Functionally closed subnet fragment (window).
 */
class SubnetView final {
public:
  using Cut = optimizer::Cut;
  using TruthTable = eda::utils::TruthTable;

  /// Constructs a subnet view corresponding to the whole subnet.
  SubnetView(const SubnetBuilder &parent);
  /// Constructs a subnet view corresponding to the maximum cone.
  SubnetView(const SubnetBuilder &parent, const size_t rootID);
  /// Constructs a subnet view corresponding to the given cut.
  SubnetView(const SubnetBuilder &parent, const Cut &cut);
  /// Constructs a subnet view corresponding to the given IO mapping.
  SubnetView(const SubnetBuilder &parent, const InOutMapping &iomapping);

  const InOutMapping &getInOutMapping() const {
    return iomapping;
  }

  size_t getInNum() const {
    return iomapping.getInNum();
  }

  size_t getOutNum() const {
    return iomapping.getOutNum();
  }

  size_t getIn(const size_t i) const {
    return iomapping.getIn(i);
  }

  size_t getOut(const size_t i) const {
    return iomapping.getOut(i);
  }

  const std::vector<size_t> &getInputs() const {
    return iomapping.inputs;
  }

  const std::vector<size_t> &getOutputs() const {
    return iomapping.outputs;
  }

  const TruthTable &getCare() const {
    return care;
  }

  void setCare(const TruthTable &care) {
    this->care = care;
  }

  /// Evaluates the truth table for the single output.
  TruthTable evaluateTruthTable() const {
    return evaluateTruthTables({getOut(0)})[0];
  }

  /// Evaluates the truth tables for the outputs.
  std::vector<TruthTable> evaluateTruthTables() const {
    return evaluateTruthTables(getOutputs());
  }

  SubnetObject &getSubnet();

  const SubnetBuilder &getParent() const {
    return parent;
  }

private:
  /// Evaluates the truth tables for the given cells.
  std::vector<TruthTable> evaluateTruthTables(
    const std::vector<size_t> &entryIDs) const;

  /// Input/output mapping.
  InOutMapping iomapping;

  /// Common care specification for all outputs.
  TruthTable care;

  /// Subnet object corresponding to the view (constructed on demand).
  SubnetObject subnet;

  /// Parent subnet builder.
  const SubnetBuilder &parent;
};

//===----------------------------------------------------------------------===//
// Subnet View Walker
//===----------------------------------------------------------------------===//

/**
 * @brief Traverses a subnet view in topological order.
 */
class SubnetViewWalker final {
public:
  /// Direction of traversal.
  enum Direction {
    /// Direct topological order: from inputs to outputs.
    FORWARD,
    /// Revese topological order: from outputs to inputs.
    BACKWARD
  };

  using ArityProvider =
      std::function<size_t(
                        SubnetBuilder &builder,
                        const size_t entryID)>;
  using LinkProvider =
      std::function<Subnet::Link(
                        SubnetBuilder &builder,
                        const size_t entryID,
                        const size_t linkIdx)>;
  using Visitor =
      std::function<bool(
                        SubnetBuilder &builder,
                        const bool isIn,
                        const bool isOut,
                        const size_t entryID)>;

  /// Traversal entry.
  struct Entry final {
    Entry(const bool isIn, const bool isOut, const uint32_t entryID):
        isIn(isIn), isOut(isOut), entryID(entryID) {}

    bool isIn;
    bool isOut;
    uint32_t entryID;
  };

  /// Ordered sequence of entries.
  using Entries = std::vector<Entry>;

  explicit SubnetViewWalker(const SubnetView &view);

  SubnetViewWalker(const SubnetView &view,
                   const ArityProvider arityProvider,
                   const LinkProvider linkProvider);

  /**
   * @brief Visits the subnet cells in direct topological order.
   *
   * Implementation is based on the backward depth-first search (BDFS).
   * The main visitor is called when a cell is popped from the BDFS stack.
   *
   * @param onBackwardDfsPop  Called on each pop from the BDFS stack.
   *                          All inputs and outputs are visited (unless abort).
   * @param onBackwardDfsPush Called on each push to the BDFS stack.
   *                          All outputs are visited; inputs are ignored.
   * @param saveEntries       Indicates whether to save entries to optimize
   *                          next runs (if required).
   *
   * @return false iff traversal is aborted.
   */
  bool runForward(const Visitor onBackwardDfsPop,
                  const Visitor onBackwardDfsPush,
                  const bool saveEntries = false);

  /// Visits the subnet cells in direct topological order.
  bool runForward(const Visitor visitor, const bool saveEntries = false) {
    return runForward(visitor, nullptr, saveEntries);
  }

  /// Visits the subnet cells in reverse topological order.
  bool runBackward(const Visitor visitor, const bool saveEntries = false);

  /// Visits the subnet cells in direct or reverse topological order.
  bool run(const Visitor visitor,
           const Direction direction = FORWARD,
           const bool saveEntries = false) {
    return (direction == FORWARD)
        ? runForward(visitor, saveEntries)
        : runBackward(visitor, saveEntries);
  }

  /// Returns the saved entries to fasten traversal.
  const Entries &getSavedEntries() const {
    assert(entries);
    return *entries;
  }

  /// Resets the entries.
  void resetSavedEntries() {
    entries = nullptr;
  }

private:
  const SubnetView &view;

  const ArityProvider arityProvider;
  const LinkProvider linkProvider;

  /// Ordered sequence of entries to fasten multiple traversals (if required).
  std::unique_ptr<Entries> entries{nullptr};
};

} // namespace eda::gate::model
