//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/visitor.h"

namespace eda::gate::optimizer {

 /**
  * \brief Visitor class to calculate every subnet depth.
  */
  class DepthFindVisitor : public Visitor {
  public:
    using GateDMap = std::unordered_map<GateId, int>;
    using Gate = model::Gate;

    /**
     * @param gateDepth Map of depts of gates.
     * @param net GNet where is required to find depths of gates.
     */
    DepthFindVisitor(GateDMap &gateDepth, const GNet *net);

    int getNetDepth() const;

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

  private:

    GateDMap &gateDepth;
    const GNet *net;
  };
} // namespace eda::gate::optimizer
