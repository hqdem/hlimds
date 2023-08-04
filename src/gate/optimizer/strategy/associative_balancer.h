//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/depth_find_visitor.h"
#include "gate/optimizer/strategy/associative_balance_visitor.h"
#include "gate/optimizer/walker.h"

namespace eda::gate::optimizer {

  /**
   * \brief Changes sequence of associative operations reducing depth
   * of the net.
   */
  class AssociativeBalancer {
  public:
    using GateDMap = DepthFindVisitor::GateDMap;

    AssociativeBalancer(GNet *);

    void balance();

  private:
    GNet *net;
  };

} // namespace eda::gate::optimizer
