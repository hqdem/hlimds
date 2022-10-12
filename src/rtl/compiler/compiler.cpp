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

  _gateIds.clear();

  for (const auto *vnode: net.vnodes()) {
    allocGates(vnode, *gnet);
  }

  for (const auto *vnode: net.vnodes()) {
    switch (vnode->kind()) {
    case VNode::SRC:
      synthSrc(vnode, *gnet);
      break;
    case VNode::VAL:
      synthVal(vnode, *gnet);
      break;
    case VNode::FUN:
      synthFun(vnode, *gnet);
      break;
    case VNode::MUX:
      synthMux(vnode, *gnet);
      break;
    case VNode::REG:
      synthReg(vnode, *gnet);
      break;
    }
  }

  return gnet;
}

Gate::Id Compiler::gateId(VNode::Id vnodeId) const {
  const auto i = _gateIds.find(vnodeId);
  if (i != _gateIds.end()) {
    return i->second;
  }

  return Gate::INVALID;
}

void Compiler::allocGates(const VNode *vnode, GNet &net) {
  assert(vnode != nullptr);

  const auto i = _gateIds.find(vnode->id());
  if (i == _gateIds.end()) {
    _gateIds.insert({ vnode->id(), Gate::nextId() });
  }

  const auto size = vnode->var().type().width();
  for (unsigned i = 0; i < size; i++) {
    net.newGate();
  }
}

void Compiler::synthSrc(const VNode *vnode, GNet &net) {
  // Do nothing.
}

void Compiler::synthVal(const VNode *vnode, GNet &net) {
  _library.synth(out(vnode), vnode->value(), net);
}

void Compiler::synthFun(const VNode *vnode, GNet &net) {
  assert(_library.supports(vnode->func()));
  _library.synth(vnode->func(), out(vnode), in(vnode), net);
}

void Compiler::synthMux(const VNode *vnode, GNet &net) {
  assert(_library.supports(FuncSymbol::MUX));
  _library.synth(FuncSymbol::MUX, out(vnode), in(vnode), net);
}

void Compiler::synthReg(const VNode *vnode, GNet &net) {
  // Level (latch), edge (flip-flop), or edge and level (flip-flop /w set/reset).
  assert(vnode->nSignals() == 1 || vnode->nSignals() == 2);

  GNet::SignalList control;
  for (const auto &event: vnode->signals()) {
    control.push_back(GNet::Signal(event.event(), gateId(event.node())));
  }

  _library.synth(out(vnode), in(vnode), control, net);
}

GNet::In Compiler::in(const VNode *vnode) {
  GNet::In in(vnode->arity());
  for (size_t i = 0; i < vnode->arity(); i++) {
    in[i] = out(VNode::get(vnode->input(i).node()));
  }

  return in;
}

GNet::Out Compiler::out(const VNode *vnode) {
  const auto base = gateId(vnode->id());
  const auto size = vnode->var().type().width();
  assert(base != Gate::INVALID);

  GNet::Out out(size);
  for (unsigned i = 0; i < size; i++) {
    out[i] = base + i;
  }

  return out;
}

} // namespace eda::rtl::compiler
