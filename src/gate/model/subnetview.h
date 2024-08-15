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
  using Cut = optimizer::CutExtractor::Cut;
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
 * @brief DFS subnet-view walker.
 */
class SubnetViewWalker final {
public:
  /// Direction of traversal.
  enum Direction { FORWARD, BACKWARD };

  /// Returns false to exit traveral.
  using Visitor = std::function<bool(SubnetBuilder &builder,
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

  /// Constructs a subnet-view walker.
  SubnetViewWalker(const SubnetView &view): view(view) {}

  /// Visits the cells of the subnet view in topological order.
  bool run(const Visitor visitor,
           const Direction direction = FORWARD,
           const bool saveEntries = false);

private:
  /// Subnet view being traversed.
  const SubnetView &view;
  /// Ordered sequence of entries to fasten multiple traversals (if required).
  std::unique_ptr<Entries> entries{nullptr};
};

} // namespace eda::gate::model
