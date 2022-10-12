//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "rtl/model/pnode.h"
#include "rtl/model/vnode.h"

namespace eda::rtl::model {

/**
 * \brief An intermediate representation combining P- and V-nets.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Net final {
public:
  using VNodeId = VNode::Id;
  using VNodeIdList = std::vector<VNodeId>;
  using Signal = VNode::Signal;
  using SignalList = VNode::SignalList;

  Net(): _created(false) {
    const size_t N = 1024*1024;

    _vnodes.reserve(N);
    _pnodes.reserve(N);
    _vnodesTemp.reserve(N);
  } 

  size_t vsize() const { return _vnodes.size(); }
  const VNode::List &vnodes() const { return _vnodes; }

  size_t psize() const { return _pnodes.size(); }
  const PNode::List &pnodes() const { return _pnodes; }

  /// Creates and adds a S-node (S = source).
  VNodeId addSrc(const Variable &var) {
    return addVNode(VNode::SRC, var, {}, FuncSymbol::NOP, {}, {});
  }

  /// Creates and adds a C-node (C = constant).
  VNodeId addVal(const Variable &var, const std::vector<bool> value) {
    return addVNode(VNode::VAL, var, {}, FuncSymbol::NOP, {}, value);
  }

  /// Creates and adds an F-node (S = function).
  VNodeId addFun(const Variable &var, FuncSymbol func, const SignalList &inputs) {
    return addVNode(VNode::FUN, var, {}, func, inputs, {});
  }

  /// Creates and adds a Phi-node (unspecified multiplexor).
  VNodeId addPhi(const Variable &var) {
    return addVNode(VNode::MUX, var, {}, FuncSymbol::NOP,  {}, {});
  }

  /// Creates and adds an M-node (M = multiplexor).
  VNodeId addMux(const Variable &var, const SignalList &inputs) {
    return addVNode(VNode::MUX, var, {}, FuncSymbol::NOP, inputs, {});
  }

  /// Creates and adds an R-node (R = register).
  VNodeId addReg(const Variable &var, const Signal &input) {
    return addVNode(VNode::REG, var, {}, FuncSymbol::NOP, { input }, {});
  }

  /// Creates and adds a combinational P-node.
  PNode *addCmb(const VNode::List &guard,
                const VNode::List &action) {
    return addPNode(new PNode(guard, action));
  }

  /// Creates and adds a sequential P-node.
  PNode *addSeq(const Signal &signal,
                const VNode::List &guard,
                const VNode::List &action) {
    return addPNode(new PNode(signal, guard, action));
  }

  /// Updates the given V-node.
  void update(VNodeId vnodeId, const SignalList &inputs) {
    auto *vnode = VNode::get(vnodeId);
    vnode->replaceWith(vnode->kind(), vnode->var(), vnode->signals(),
                        vnode->func(), inputs, vnode->value());
  }

  /// Creates the V-net according to the P-net
  /// (after that any changes are prohibited).
  void create();

  //===--------------------------------------------------------------------===//
  // Graph Interface (only V-Net)
  //===--------------------------------------------------------------------===//

  /*
  using V = GateId;
  using E = Link;

  /// Returns the number of nodes.
  size_t nNodes() const {
    return nGates();
  }

  /// Returns the number of edges.
  size_t nEdges() const {
    return nConnects();
  }

  /// Checks whether the graph contains the node.
  bool hasNode(GateId gid) const {
    return contains(gid);
  }

  /// Checks whether the graph contains the edge.
  bool hasEdge(const Link &link) const {
    return !hasTrigger(link.target);
  }

  /// Returns the graph sources.
  GateIdSet getSources() const {
    GateIdSet sources;
    sources.reserve(_sourceLinks.size() + _triggers.size());

    for (auto link : _sourceLinks) {
      sources.insert(link.source);
    }
    for (auto trigger : _triggers) {
      sources.insert(trigger);
    }

    return sources;
  }

  /// Returns the outgoing edges of the node.
  const LinkList &getOutEdges(GateId gid) const {
    return Gate::get(gid)->links();
  }

  /// Returns the end of the edge.
  GateId leadsTo(const Link &link) const {
    return link.target;
  }
  */

private:
  void muxWireDefines(VNode *phi, const VNode::List &defines);

  void muxRegDefines(VNode *phi, const VNode::List &defines);

  std::vector<std::pair<Signal, VNode::List>> groupRegDefines(const VNode::List &defines);

  VNode *createMux(const Variable &output, const VNode::List &defines);

  VNodeId addVNode(VNode::Kind kind,
                   const Variable &var,
                   const SignalList &signals,
                   FuncSymbol func,
                   const SignalList &inputs,
                   const std::vector<bool> &value) {
    return addVNode(new VNode(kind, var, signals, func, inputs, value));
  }
 
  VNodeId addVNode(VNode *vnode) {
    assert(!_created);
    auto &usage = _vnodesTemp[vnode->var().name()];
    if (vnode->kind() == VNode::MUX) {
      usage.first = vnode;
    } else {
      usage.second.push_back(vnode);
    }
    return vnode->id();
  }

  PNode *addPNode(PNode *pnode) {
    assert(!_created);
    _pnodes.push_back(pnode);
    return pnode;
  }

  VNode::List _vnodes;
  PNode::List _pnodes;
 
  /// Maps a variable x to the <phi(x), {def(x), ..., def(x)}> structure.
  std::unordered_map<std::string, std::pair<VNode*, VNode::List>> _vnodesTemp;

  bool _created;
};

std::ostream& operator <<(std::ostream &out, const Net &net);

} // namespace eda::rtl::model
