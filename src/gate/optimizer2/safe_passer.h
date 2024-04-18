//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

namespace eda::gate::optimizer2 {

using EntryIterator = eda::gate::model::EntryIterator;

/**
 * @brief Safe entries passer that avoids entries added by replacing in previous
 * iterations.
 */
class SafePasser : public EntryIterator {

  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID = eda::gate::model::SubnetID;

public:
  SafePasser() = delete;
  /**
   * @brief Constructs a SafePasser.
   *
   * @param iter Base EntryIterator used to create SafePasser.
   */
  SafePasser(EntryIterator iter) :
      EntryIterator(iter),
      builderToTransform(const_cast<SubnetBuilder *>(builder)) {};

  SafePasser &operator++();
  SafePasser operator++(int);
  SafePasser &operator--();
  SafePasser operator--(int);

  /**
   * @brief SubnetBuilder::replace(...) wrapper that allows to maintain next
   * passer iterations safe.
   */
  void replace(
      const SubnetID rhsID,
      std::unordered_map<size_t, size_t> &rhsToLhs,
      const std::function<float(const size_t /* index in subnet */)> *getCellWeight = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onNewCell = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onEqualDepth = nullptr,
      const std::function<void(const size_t /* index in builder */)> *onGreaterDepth = nullptr);

  /// Clears information about unsafe entries in subnet builder.
  void finalizePass();

private:
  SubnetBuilder *builderToTransform;

  std::vector<char> isNewEntry{};

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

} // namespace eda::gate::optimizer2
