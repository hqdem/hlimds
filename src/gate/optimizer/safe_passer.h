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

  using Subnet = eda::gate::model::Subnet;
  using SubnetID = eda::gate::model::SubnetID;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetObject = eda::gate::model::SubnetObject;

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
    const std::function<void(const size_t)> *onEachEntry = nullptr) :
      EntryIterator(iter),
      builderToTransform(const_cast<SubnetBuilder *>(builder)),
      onEachEntry(onEachEntry) {

    if (onEachEntry) {
      (*onEachEntry)(entry);
    }
  };

  SafePasser &operator=(const SafePasser &other) = delete;
  SafePasser &operator=(SafePasser &&other) = default;
  SafePasser(SafePasser &&other) = default;

protected:
  SafePasser(const SafePasser &other) = default;

public:
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
      const SubnetBuilder::InOutMapping &rhsToLhsMapping,
      const std::function<void(const size_t /* index in builder */)> *onNewCell = nullptr, // FIXME: Use type aliases defined in SubnetBuilder.
      const std::function<void(const size_t /* index in builder */)> *onEqualDepth = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onGreaterDepth = nullptr);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe. Version with SubnetID rhs.
   */
  void replace(
      const SubnetID rhsID,
      const SubnetBuilder::InOutMapping &rhsToLhsMapping,
      const std::function<float(const size_t /* index in subnet */)> *getCellWeight = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onNewCell = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onEqualDepth = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onGreaterDepth = nullptr);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe. Version with SubnetBuilder rhs.
   */
  void replace(
      const SubnetBuilder &rhsBuilder,
      const SubnetBuilder::InOutMapping &rhsToLhsMapping,
      const std::function<void(const size_t /* index in builder */)> *onNewCell = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onEqualDepth = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onGreaterDepth = nullptr);

  /// Clears information about unsafe entries in subnet builder.
  void finalizePass();

private:
  /// Checks replacement possibility and saves old cone root next entry.
  void prepareForReplace(
      const size_t rhsOutEntryID,
      const SubnetBuilder::InOutMapping &rhsToLhsMapping) {
    assert(/* Replacing unsafe root entry */
           isNewEntry.size() <= entry || !isNewEntry[entry]);
    assert(/* Current passer entry and rhs root entry differs */
           rhsToLhsMapping.getOut(0) == entry);

    saveRoot = entry;
    EntryIterator::operator++();
    saveNext = entry;
    EntryIterator::operator--();
  }

  inline void callOnEachCell() {
    if (onEachEntry && entry != SubnetBuilder::upperBoundID &&
        entry != SubnetBuilder::lowerBoundID) {
      (*onEachEntry)(entry);
    }
  }

private:
  SubnetBuilder *builderToTransform;

  std::vector<char> isNewEntry{};
  const std::function<void(const size_t)> *onEachEntry;

  value_type saveNext{SubnetBuilder::invalidID};
  value_type saveRoot{SubnetBuilder::invalidID};
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
