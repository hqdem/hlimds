//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/visitor.h"

namespace eda::gate::optimizer {

 /**
  * \brief Finds cuts in given net.
  */
  class CutsFindVisitor : public Visitor {

    int cutSize;
    CutStorage *cutStorage;

  public:

    /**
     * @param cutSize Max number of nodes in a cut.
     * @param cutStorage Struct where cuts are stored.
     */
    CutsFindVisitor(int cutSize, CutStorage *cutStorage);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

  };
} // namespace eda::gate::optimizer
