//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_visitor.h"
#include "gate/optimizer/walker.h"

namespace eda::gate::optimizer {

 /**
  * \brief Class traces nodes in topological order and their cuts.
  * \ Calls visitor to handle each node and cut.
  */
  class CutWalker : public Walker {

    CutStorage *cutStorage;
    CutVisitor *cutVisitor;

    VisitorFlags callVisitor(GateId node) override;

  public:
    /**
     * @param gNet Net that has to be traced.
     * @param visitor Some implementation of visitor to handle nodes and cuts.
     * @param cutStorage Set of cuts for all nodes.
     */
    CutWalker(GNet *gNet, CutVisitor *visitor, CutStorage *cutStorage);
  };

} // namespace eda::gate::optimizer
