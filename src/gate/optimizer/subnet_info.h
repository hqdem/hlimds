//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/subnet_basis.h"

namespace eda::gate::optimizer {

/**
 * \brief Stores subnet characteristics.
 * Stores number of input, output and inner cells, depth, max arity and
 * basis (`SubnetBasis` struct).
 */
struct SubnetInfo {
  model::SubnetSz inNum, outNum, innerNum;
  model::SubnetDepth depth;
  uint16_t maxArity;
  SubnetBasis basis;

  static SubnetInfo makeEmpty() {
    SubnetInfo info;
    info.basis    = 0;
    info.inNum    = 0;
    info.outNum   = 0;
    info.innerNum = 0;
    info.depth    = 0;
    info.maxArity = 0;
    return info;
  }
};

} // namespace eda::gate::optimizer
