//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/model/vnode.h"
#include "util/string.h"

#include <cassert>
#include <cstddef>
#include <memory>

using namespace eda::gate::model;
using namespace eda::rtl::library;
using namespace eda::rtl::model;
using namespace eda::utils;

namespace eda::rtl::compiler {

std::unique_ptr<GNet> Compiler::compile(const Net &net) {
  auto gnet = std::make_unique<GNet>();

  _gates_id.clear();

  for (const auto *vnode: net.vnodes()) {
    alloc_gates(vnode, *gnet);
  }

  for (const auto *vnode: net.vnodes()) {
    switch (vnode->kind()) {
    case VNode::SRC:
      synth_src(vnode, *gnet);
      break;
    case VNode::VAL:
      synth_val(vnode, *gnet);
      break;
    case VNode::FUN:
      synth_fun(vnode, *gnet);
      break;
    case VNode::MUX:
      synth_mux(vnode, *gnet);
      break;
    case VNode::REG:
      synth_reg(vnode, *gnet);
      break;
    }
  }

  return gnet;
}

Gate::Id Compiler::gate_id(const VNode *vnode) const {
  const auto i = _gates_id.find(vnode->name());
  if (i != _gates_id.end()) {
    return i->second;
  }

  return -1u;
}

Gate::Id Compiler::gate_id(const VNode *vnode, const GNet &net) {
  const auto id = gate_id(vnode);
  if (id != -1u) {
    return id;
  }

  _gates_id.insert({ vnode->name(), net.size() });
  return net.size();
}

void Compiler::alloc_gates(const VNode *vnode, GNet &net) {
  assert(vnode != nullptr);

  const auto size = vnode->var().type().width();
  for (unsigned i = 0; i < size; i++) {
    net.addGate();
  }
}

void Compiler::synth_src(const VNode *vnode, GNet &net) {
  // Do nothing.
}

void Compiler::synth_val(const VNode *vnode, GNet &net) {
  _library.synthesize(out(vnode), vnode->value(), net);
}

void Compiler::synth_fun(const VNode *vnode, GNet &net) {
  assert(_library.supports(vnode->func()));
  _library.synthesize(vnode->func(), out(vnode), in(vnode), net);
}

void Compiler::synth_mux(const VNode *vnode, GNet &net) {
  assert(_library.supports(FuncSymbol::MUX));
  _library.synthesize(FuncSymbol::MUX, out(vnode), in(vnode), net);
}

void Compiler::synth_reg(const VNode *vnode, GNet &net) {
  // Level (latch), edge (flip-flop), or edge and level (flip-flop /w set/reset).
  assert(vnode->esize() == 1 || vnode->esize() == 2);

  Signal::List control;
  for (const auto &event: vnode->events()) {
    control.push_back(Signal(event.kind(), gate_id(event.node())));
  }

  _library.synthesize(out(vnode), in(vnode), control, net);
}

GNet::In Compiler::in(const VNode *vnode) {
  GNet::In in(vnode->arity());
  for (std::size_t i = 0; i < vnode->arity(); i++) {
    in[i] = out(vnode->input(i));
  }

  return in;
}

GNet::Out Compiler::out(const VNode *vnode) {
  const auto base = gate_id(vnode);
  const auto size = vnode->var().type().width();
  assert(base != -1u);

  GNet::Out out(size);
  for (unsigned i = 0; i < size; i++) {
    out[i] = base + i;
  }

  return out;
}

} // namespace eda::rtl::compiler
