//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"

namespace eda::gate::optimizer {

  // CONTINUE - flag of success, keeps iteration as it has to go.
  // SKIP - makes to begin iteration for the next node.
  // FINISH_ALL_NODES - stops iteration for all nodes and cuts.
  // FINISH_FURTHER_NODES - stops iteration for all child nodes
  // of the current node.

  enum VisitorFlags {
    CONTINUE,
    SKIP,
    FINISH_ALL_NODES,
    FINISH_FURTHER_NODES
  };

 /**
  * \brief Interface to handle node and its cuts.
  */
  class Visitor {

  public:
    using GNet = model::GNet;
    using GateId = GNet::GateId;
    using Cut = CutStorage::Cut;
    using MatchMap = std::unordered_map<GateId, GateId>;

    /**
     * Starts handling a tracing node.
     */
    virtual VisitorFlags onNodeBegin(const GateId &) = 0;

    /**
     * Finishes handling a tracing node.
     */
    virtual VisitorFlags onNodeEnd(const GateId &) = 0;
  };
} // namespace eda::gate::optimizer
