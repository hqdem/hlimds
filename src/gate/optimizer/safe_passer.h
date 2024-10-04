//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

namespace eda::gate::optimizer {

using EntryIterator = eda::gate::model::EntryIterator;

/**
 * @brief Safe entries passer that avoids entries added by replacing in previous
 * iterations.
 */
class SafePasser : public EntryIterator {

  using InOutMapping = eda::gate::model::InOutMapping;
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = eda::gate::model::SubnetID;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetObject = eda::gate::model::SubnetObject;
  using CellActionCallback = SubnetBuilder::CellActionCallback;
  using CellCallbackCondition =
      std::function<void(const uint32_t, const uint32_t, const uint32_t)>;
  using CellWeightProvider = SubnetBuilder::CellWeightProvider;

  enum Direction {
    FORWARD,
    BACKWARD,
    UNDEF
  };

public:
  SafePasser() = delete;
  /**
   * @brief Constructs a SafePasser.
   *
   * @param iter Base EntryIterator used to create SafePasser.
   * @param onEachEntry Callback that is executed on each entry during iteration
   * (including skipped unsafe entries).
   */
  SafePasser(
    EntryIterator iter,
    const std::function<void(const uint32_t)> *onEachEntry = nullptr);

  SafePasser &operator=(const SafePasser &other) = delete;
  SafePasser &operator=(SafePasser &&other) = default;
  SafePasser(SafePasser &&other) = default;

protected:
  SafePasser(const SafePasser &other) = default;

public:
  void changeItParent(const Direction dir);
  void changeIt(const Direction dir);
  SafePasser &operator++();
  SafePasser operator++(int);
  SafePasser &operator--();
  SafePasser operator--(int);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe.
   */
  void replace(
      const SubnetObject &rhs,
      const InOutMapping &rhsToLhsMapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellCallbackCondition *onRecomputedDepth = nullptr);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe. Version with SubnetID rhs.
   */
  void replace(
      const SubnetID rhsID,
      const InOutMapping &rhsToLhsMapping,
      const CellWeightProvider *getCellWeight = nullptr,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellCallbackCondition *onRecomputedDepth = nullptr);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe. Version with SubnetBuilder rhs.
   */
  void replace(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &rhsToLhsMapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellCallbackCondition *onRecomputedDepth = nullptr);

  /// Clears information about unsafe entries in subnet builder.
  void finalizePass();

private:
  /// Checks replacement possibility and saves old cone root next entry.
  void prepareForReplace(
      const uint32_t rhsOutEntryID,
      const InOutMapping &rhsToLhsMapping) {
    assert(/* Replacing unsafe root entry */
           isNewEntry.size() <= entry || !isNewEntry[entry]);
    assert(/* Current passer entry and rhs root entry differs */
           rhsToLhsMapping.getOut(0).idx == entry);

    EntryIterator::operator++();
    saveNext = entry;
    EntryIterator::operator--();
  }

  /// Recomputes saveNext according to the passer direction and new root depth.
  void recomputeNext(const uint32_t oldRootDepth, const bool rootLastDepth) {
    if (direction == FORWARD) {
      const auto curRootDepth = builder->getDepth(entry);
      if (oldRootDepth < curRootDepth && rootLastDepth) {
        saveNext = builder->getFirstWithDepth(oldRootDepth + 1);
      }
    } else if (direction == BACKWARD) {
      const auto curRootDepth = builder->getDepth(entry);
      if (oldRootDepth > curRootDepth && rootLastDepth) {
        uint32_t depthToCheck = oldRootDepth;
        while (builder->getLastWithDepth(depthToCheck) ==
               SubnetBuilder::invalidID) {
          depthToCheck--;
        }
        saveNext = builder->getLastWithDepth(depthToCheck);
      } else if (oldRootDepth < curRootDepth && rootLastDepth) {
        saveNext = builder->getLastWithDepth(oldRootDepth);
      }
    }
  }

  inline void callOnEachCell() {
    if (onEachEntry && entry != SubnetBuilder::upperBoundID &&
        entry != SubnetBuilder::lowerBoundID) {
      (*onEachEntry)(entry);
    }
  }

  void checkDirection(Direction dir) {
    assert(dir != UNDEF);
    assert(dir == this->direction || this->direction == UNDEF);
    this->direction = dir;
  }

private:
  SubnetBuilder *builderToTransform;

  Direction direction{UNDEF};

  std::vector<char> isNewEntry{};
  std::vector<char> isPassedEntry{};
  const std::function<void(const uint32_t)> *onEachEntry;

  value_type saveNext{SubnetBuilder::invalidID};
};

/**
 * @brief Reverse version of SafePasser.
 */
class ReverseSafePasser final : public SafePasser {
public:
  ReverseSafePasser() = delete;
  /**
   * @brief Constructs a ReverseSafePasser.
   *
   * @param iter Reverse EntryIterator used to create ReverseSafePasser.
   */
  ReverseSafePasser(std::reverse_iterator<EntryIterator> iter) :
      SafePasser(--iter.base()) {};

  ReverseSafePasser &operator++();
  ReverseSafePasser operator++(int);
  ReverseSafePasser &operator--();
  ReverseSafePasser operator--(int);
};

} // namespace eda::gate::optimizer
