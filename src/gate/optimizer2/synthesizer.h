//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <gate/model2/subnet.h>

#include <kitty/dynamic_truth_table.hpp>

namespace eda::gate::optimizer2 {

/**
 * \brief Common interface for synthesizers.
 */
class Synthesizer {
protected:
  using SubnetID = eda::gate::model::SubnetID;
  using TruthTable = kitty::dynamic_truth_table;

  virtual SubnetID synthesize(const TruthTable &func) = 0;
};

} // namespace eda::gate::optimizer2