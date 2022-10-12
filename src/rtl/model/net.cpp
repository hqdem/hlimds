//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "rtl/model/net.h"
#include "util/string.h"

#include <cassert>
#include <cstddef>

using namespace eda::utils;

namespace eda::rtl::model {
 
void Net::create() {
  assert(!_created);

  for (auto &[_, usage]: _vnodes_temp) {
    assert(!_.empty());

    VNode *phi = usage.first;
    VNode::List &defines = usage.second;

    // Multiple definitions <=> phi-node is required.
    assert((phi != nullptr && defines.size() >= 2) ||
           (phi == nullptr && defines.size() == 1));

    // For registers, the node is updated even for a single definition:
    // it is supplemented w/ the signal triggering the parent p-node.
    phi = (phi != nullptr ? phi : defines.front());

    switch (phi->var().kind()) {
    case Variable::WIRE:
      mux_wire_defines(phi, defines);
      break;
    case Variable::REG:
      mux_reg_defines(phi, defines);
      break;
    }
  }

  _vnodes_temp.clear();
  _created = true;
}

// if (g[1]) { w <= f[1](...) }    w[1] <= f[1](...)
// ...                          => ...               + w <= mux{ g[i] -> w[i] }
// if (g[n]) { w <= f[n](...) }    w[n] <= f[n](...)
void Net::mux_wire_defines(VNode *phi, const VNode::List &defines) {
  const size_t n = defines.size();
  assert(n > 0);

  // No multiplexing is required.
  if (n == 1) {
    _vnodes.push_back(defines.front());
    return;
  }

  // Create the { w[i] } nodes and compose the mux inputs: { g[i] -> w[i] }.
  SignalList inputs(2 * n);

  for (size_t i = 0; i < n; i++) {
    VNode *old_vnode = defines[i];

    assert(old_vnode->pnode() != nullptr);
    assert(old_vnode->pnode()->gsize() > 0);

    // Create a { w[i] <= f[i](...) } node.
    VNode *new_vnode = old_vnode->duplicate(unique_name(old_vnode->name()));
    _vnodes.push_back(new_vnode);

    // Guards come first: mux(g[1], ..., g[n]; w[1], ..., w[n]).
    inputs[i] = old_vnode->pnode()->guard().back()->always();
    inputs[i + n] = new_vnode->always();
  }

  // Connect the wire w/ the multiplexor: w <= mux{ g[i] -> w[i] }.
  Variable output = phi->var();
  phi->replace_with(VNode::MUX, output, {}, FuncSymbol::NOP, inputs, {});

  _vnodes.push_back(phi);
}

// @(signal): if (g[1]) { r <= w[1] }    w <= mux{ g[i] -> w[i] }
// ...                     =>
// @(signal): if (g[n]) { r <= w[n] }    @(signal): r <= w
void Net::mux_reg_defines(VNode *phi, const VNode::List &defines) {
  std::vector<std::pair<Signal, VNode::List>> groups = group_reg_defines(defines);

  Variable output = phi->var();

  SignalList signals;
  SignalList inputs;

  for (const auto &[signal, defines]: groups) {
    // Create a wire w for the given signal.
    const VNode *vnode = VNode::get(signal.node());
    const std::string name = output.name() + "$" + vnode->name();
    Variable wire(name, Variable::WIRE, output.type());

    // Create a multiplexor: w <= mux{ g[i] -> w[i] }.
    VNode *mux = create_mux(wire, defines);
    _vnodes.push_back(mux);

    signals.push_back(signal);
    inputs.push_back(mux->always());
  }

  // Connect the register w/ the multiplexor(s) via the wire(s): r <= w.
  phi->replace_with(VNode::REG, output, signals, FuncSymbol::NOP, inputs, {});
  _vnodes.push_back(phi);
}

std::vector<std::pair<VNode::Signal, VNode::List>> Net::group_reg_defines(const VNode::List &defines) {
  const Signal *clock = nullptr;
  const Signal *level = nullptr;

  VNode::List clock_defines;
  VNode::List level_defines;

  // Collect all the signals triggering the register.
  for (VNode *vnode: defines) {
    assert(vnode != nullptr && vnode->pnode() != nullptr);

    const Signal &signal = vnode->pnode()->signal();
    assert(signal.isEdge() || signal.isLevel());

    if (signal.isEdge()) {
      // At most one edge-triggered signal (clock) is allowed.
      assert(clock == nullptr || *clock == signal);
      clock = &signal;
      clock_defines.push_back(vnode);
    } else {
      // At most one level-triggered signal (enable or reset) is allowed.
      assert(level == nullptr || *level == signal);
      level = &signal;
      level_defines.push_back(vnode);
    }
  }

  std::vector<std::pair<Signal, VNode::List>> groups;
  if (clock != nullptr) {
    groups.push_back({ *clock, clock_defines });
  }
  if (level != nullptr) {
    groups.push_back({ *level, level_defines });
  }

  return groups;
}

VNode *Net::create_mux(const Variable &output, const VNode::List &defines) {
  const size_t n = defines.size();
  assert(n != 0);

  // Multiplexor is not required.
  if (n == 1) {
    VNode *vnode = defines.front();
    return new VNode(VNode::FUN, output, {}, FuncSymbol::NOP, { vnode->input(0) }, {}); 
  }

  // Compose the mux inputs { g[i] -> w[i] }.
  SignalList inputs(2 * n);

  for (size_t i = 0; i < n; i++) {
    VNode *vnode = defines[i];
    assert(vnode->pnode() != nullptr);

    // Guards come first: mux(g[1], ..., g[n]; w[1], ..., w[n]).
    inputs[i] = vnode->pnode()->guard().back()->always();
    inputs[i + n] = vnode->input(0);
  }

  // Create a multiplexor: w <= mux{ g[i] -> w[i] }.
  return new VNode(VNode::MUX, output, {}, FuncSymbol::NOP, inputs, {});
}

std::ostream &operator <<(std::ostream &out, const Net &net) {
  for (const auto *pnode: net.pnodes()) {
    out << *pnode << std::endl;
  }

  for (const auto *vnode: net.vnodes()) {
    out << *vnode << std::endl;
  }

  return out;
}
 
} // namespace eda::rtl::model
