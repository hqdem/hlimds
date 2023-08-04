//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/visitor.h"

#pragma once

namespace eda::gate::optimizer {

 /**
  * \brief For each node elaborates all cuts of the node.
  */
  class CutVisitor : public Visitor {

  public:

    /**
     * Cut handle method.
     * @param gate Currently handling gate.
     * @param cut Currently handling cut of the gate.
     */
    virtual VisitorFlags onCut(const GateId &gate, const Cut &cut) = 0;

  };

} // namespace eda::gate::optimizer
