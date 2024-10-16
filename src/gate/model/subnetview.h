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
#include "gate/optimizer/cut.h"
#include "util/truth_table.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet View
//===----------------------------------------------------------------------===//

class SubnetView;
class SubnetViewWalker;

/// SubnetView forward iterator.
class SubnetViewIter final {
  friend class SubnetView;
  friend class SubnetViewWalker;

  enum ConstructionT {
    /// To construct SubnetView::begin().
    BEGIN,
    /// To construct SubnetView::end().
    END
  };

public:
  typedef EntryID value_type;
  typedef std::ptrdiff_t difference_type;
  typedef const value_type *pointer;
  typedef const value_type &reference;
  typedef std::forward_iterator_tag iterator_category;

  using Visitor =
      std::function<bool(
                         SubnetBuilder &builder,
                         const bool isIn,
                         const bool isOut,
                         const EntryID entryID)>;
  using ArityProvider =
      std::function<uint16_t(
                        SubnetBuilder &builder,
                        const EntryID entryID)>;
  using LinkProvider =
      std::function<Subnet::Link(
                        SubnetBuilder &builder,
                        const EntryID entryID,
                        const uint16_t linkIdx)>;

private:
  static inline uint16_t defaultArityProvider(
      SubnetBuilder &builder, const EntryID entryID) {
    return builder.getCell(entryID).arity;
  }

  static inline Subnet::Link defaultLinkProvider(
      SubnetBuilder &builder, const EntryID entryID, const uint16_t linkIdx) {
    return builder.getLink(entryID, linkIdx);
  }

  SubnetViewIter(
      const SubnetView *view,
      const ConstructionT constructionT,
      const ArityProvider arityProvider,
      const LinkProvider linkProvider,
      const Visitor *onBackwardDfsPop = nullptr,
      const Visitor *onBackwardDfsPush = nullptr);

  SubnetViewIter(
      const SubnetView *view,
      const ConstructionT constructionT,
      const Visitor *onBackwardDfsPop = nullptr,
      const Visitor *onBackwardDfsPush = nullptr);

  void prepareIteration();

  void nextPI();

public:
  bool operator==(const SubnetViewIter &other) const {
    return this->view == other.view &&
        this->entryLink.first == other.entryLink.first;
  }

  bool operator!=(const SubnetViewIter &other) const {
    return !(*this == other);
  }

  reference operator*() const;
  pointer operator->() const;

  SubnetViewIter &operator++();
  SubnetViewIter operator++(int) = delete;
  SubnetViewIter next() const;

private:
  const SubnetView *view{nullptr};
  std::pair<value_type, uint16_t> entryLink{SubnetBuilder::invalidID, 0};
  const ArityProvider arityProvider;
  const LinkProvider linkProvider;
  const Visitor *onBackwardDfsPop{nullptr};
  const Visitor *onBackwardDfsPush{nullptr};

  // Iterator state
  bool isAborted{false};
  std::unordered_set<EntryID> marked;
  std::unordered_set<EntryID> inout;
  uint32_t nInLeft{0};
  uint32_t nOutLeft{0};
  std::stack<std::pair<EntryID /* entry */, uint16_t /* link */>> stack;
};

/**
 * @brief Functionally closed subnet fragment (window).
 */
class SubnetView final {
public:
  using Cut = optimizer::Cut;
  using Link = InOutMapping::Link;
  using TruthTable = eda::util::TruthTable;

  /// Constructs a subnet view corresponding to the whole subnet.
  SubnetView(const std::shared_ptr<SubnetBuilder> &parent);
  /// Constructs a subnet view corresponding to the maximum cone.
  SubnetView(const std::shared_ptr<SubnetBuilder> &parent,
             const EntryID rootID);
  /// Constructs a subnet view corresponding to the given cut.
  SubnetView(const std::shared_ptr<SubnetBuilder> &parent, const Cut &cut);
  /// Constructs a subnet view corresponding to the given IO mapping.
  SubnetView(const std::shared_ptr<SubnetBuilder> &parent,
             const InOutMapping &iomapping);

  const InOutMapping &getInOutMapping() const {
    return iomapping;
  }

  uint16_t getInNum() const {
    return iomapping.getInNum();
  }

  uint16_t getOutNum() const {
    return iomapping.getOutNum();
  }

  Link getIn(const uint16_t i) const {
    return iomapping.getIn(i);
  }

  Link getOut(const uint16_t i) const {
    return iomapping.getOut(i);
  }

  const InOutMapping::LinkList &getInputs() const {
    return iomapping.inputs;
  }

  const InOutMapping::LinkList &getOutputs() const {
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

  const SubnetObject &getParent() const {
    return parent;
  }

  SubnetViewIter begin() const {
    return SubnetViewIter(this, SubnetViewIter::BEGIN);
  }

  SubnetViewIter end() const {
    return SubnetViewIter(this, SubnetViewIter::END);
  }

private:
  /// Evaluates the truth tables for the given cells.
  std::vector<TruthTable> evaluateTruthTables(
      const InOutMapping::LinkList &entryLinks) const;

  /// Input/output mapping.
  InOutMapping iomapping;

  /// Common care specification for all outputs.
  TruthTable care;

  /// Parent subnet object corresponding to the view.
  const SubnetObject parent;

  /// Subnet object corresponding to the view (construct on demand).
  SubnetObject subnet;
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

  using ArityProvider = SubnetViewIter::ArityProvider;
  using LinkProvider = SubnetViewIter::LinkProvider;
  using Visitor = SubnetViewIter::Visitor;

  /// Traversal entry.
  struct Entry final {
    Entry(const bool isIn, const bool isOut, const EntryID entryID):
        isIn(isIn), isOut(isOut), entryID(entryID) {}

    bool isIn;
    bool isOut;
    EntryID entryID;
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
   * @return false if traversal is aborted.
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
