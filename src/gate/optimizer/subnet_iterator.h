//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/model2/subnet.h"
#include "gate/optimizer/reconvergence_cut.h"

#include <unordered_map>

namespace eda::gate::optimizer {

/**
 * @brief A common interface for iteration over the subnet.
 */
class SubnetIteratorBase {
public:

  /// @cond ALIASES
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  /// @endcond

  /// A fragment of the subnet.
  struct SubnetFragment final {
    /// Id of the fragment. 
    SubnetID subnetID;
    /// Mapping the fragment to the original subnet.
    /// (fragment cell -> original subnet cell).
    std::unordered_map<size_t, size_t> entryMap;

    bool isValid() {
      return subnetID != model::OBJ_NULL_ID;
    }
  };

  /**
   * @brief Сonstructor.
   * @param subnetBuilder The subnet for iteration.
   */
  SubnetIteratorBase(const SubnetBuilder &subnetBuilder)
      : subnetBuilder(subnetBuilder) {}

  /**
   * @brief Finds the next fragment in the subnet.
   * @return The found fragment.
   */
  virtual SubnetFragment next() = 0;

  virtual ~SubnetIteratorBase() = default;

protected:

  /// The subnet for iteration.
  const SubnetBuilder &subnetBuilder;
};

} // namespace eda::gate::optimizer
