//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include "kitty/dynamic_truth_table.hpp"

namespace eda::gate::optimizer2 {

/**
 * \brief Common interface for synthesizers.
 */
template<typename IR>
class Synthesizer {
public:
  using SubnetID = eda::gate::model::SubnetID;

  Synthesizer() {}
  virtual ~Synthesizer() {}

  virtual SubnetID synthesize(const IR &ir, uint16_t maxArity) = 0;
};

} // namespace eda::gate::optimizer2
