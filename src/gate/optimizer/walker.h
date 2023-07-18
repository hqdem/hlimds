//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/util.h"
#include "gate/optimizer/visitor.h"
#include "util/graph.h"

#include <queue>

namespace eda::gate::optimizer {

 /**
  * \brief Basic walker for gate-level model.
  * \ Elaborates nodes in topological order with visitor.
  */
  class Walker {

  public:
    using GNet = eda::gate::model::GNet;
    using Gate = eda::gate::model::Gate;
    using GateID = GNet::GateId;
    using Cut = CutStorage::Cut;

  protected:
    GNet *gNet;
    Visitor *visitor;

    virtual VisitorFlags callVisitor(GateID node);

  private:
    void walk(std::queue<GateID> &start, std::unordered_set<GateID> &accessed,
              bool forward);

    void
    walkAll(std::queue<GateID> &start, const std::unordered_set<GateID> &used,
            bool forward);

    bool checkVisited(const std::unordered_set<GateID> &visited, GateID node,
                      bool forward);

    bool checkAllVisited(const std::unordered_set<GateID> &visited,
                         const std::unordered_set<GateID> &used, GateID node,
                         bool forward);

  public:
    /**
     * @param gNet Net to be traced.
     * @param visitor Node handler.
     */
    Walker(GNet *gNet, Visitor *visitor);

    /**
     * Traces all nodes in topological order and calls the handler on each node.
     * @param forward Direction to perform a trace in.
     */
    void walk(bool forward);

    /**
     * Traces nodes from a cone in topological order and calls the handler on each node.
     * Trace is performed from the cone vertex to the cone base.
     * @param start Cone vertex.
     * @param cut Cone base.
     * @param forward Direction to perform a trace in.
     */
    void walk(GateID start, const Cut &end, bool forward);

    /**
     * Traces nodes from a cone in topological order and calls the handler on each node.
     * Trace is performed from the cone base to the cone vertex.
     * @param start Cone base.
     * @param end Cone vertex.
     * @param forward Direction to perform a trace in.
     */
    void walk(const Cut &start, GateID end, bool forward);

    /**
     * Traces nodes from a cone in topological order and calls the handler on each node.
     * Trace is performed from the cone vertex to the cone base.
     * Cone base is assumed to be sources of a net.
     * @param start Cone vertex.
     * @param forward Direction to perform a trace in.
     */
    void walk(GateID start, bool forward);

    /**
     * Starts walking from the nodes from listed collection.
     * Can handle FINISH_FURTHER_NODES visitor flag.
     * @tparam L Some collection of nodes.
     * @param start Nodes with which a trace starts.
     * @param used Set of all nodes that needs be traced.
     */
    template<typename L>
    void walk(L start, const std::unordered_set<GateID> &used) {

      std::queue<GateID> bfs;
      for (const auto &node: start) {
        bfs.push(node);
      }

      walkAll(bfs, used, true);
    }

  };

} // namespace eda::gate::optimizer
