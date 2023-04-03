//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "rtl/library/flibrary.h"

#include <cassert>

using namespace eda::base::model;
using namespace eda::gate::model;
using namespace eda::rtl::model;

namespace eda::rtl::library {

bool FLibraryDefault::supports(FuncSymbol func) const {
  return true;
}

FLibrary::Out FLibraryDefault::synth(
    size_t outSize, const Value &value, GNet &net) const {
  assert(outSize == value.size());

  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = value[i] ? net.addOne() : net.addZero();
  }

  return out;
}

FLibrary::Out FLibraryDefault::synth(
    size_t outSize, const Out &out, GNet &net) const {
  assert(outSize == out.size());

  Out targets(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    targets[i] = net.addOut(out[i]);
  }

  return targets;
}

FLibrary::Out FLibraryDefault::synth(
    size_t outSize, FuncSymbol func, const In &in, GNet &net) const {
  switch (func) {
  case FuncSymbol::NOP:
    return synthUnaryBitwiseOp(GateSymbol::NOP, outSize, in, net);
  case FuncSymbol::NOT:
    return synthUnaryBitwiseOp(GateSymbol::NOT, outSize, in, net);
  case FuncSymbol::AND:
    return synthBinaryBitwiseOp(GateSymbol::AND, outSize, in, net);
  case FuncSymbol::OR:
    return synthBinaryBitwiseOp(GateSymbol::OR, outSize, in, net);
  case FuncSymbol::XOR:
    return synthBinaryBitwiseOp(GateSymbol::XOR, outSize, in, net);
  case FuncSymbol::ADD:
    return synthAdd(outSize, in, net);
  case FuncSymbol::SUB:
    return synthSub(outSize, in, net);
  case FuncSymbol::MUL:
    return synthMul(outSize, in, net);
  case FuncSymbol::MUX:
    return synthMux(outSize, in, net);
  default:
    assert(false);
    return {};
  }
}

FLibrary::Out FLibraryDefault::alloc(size_t outSize, GNet &net) const {
  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = net.newGate();
  }

  return out;
}

FLibrary::Out FLibraryDefault::synth(
    const Out &out, const In &in, const SignalList &control, GNet &net) const {
  assert(control.size() == 1 || control.size() == 2);
  assert(control.size() == in.size());

  auto clock = invertIfNegative(control[0], net);

  if (control.size() == 1) {
    const auto &x = in[0];
    assert(out.size() == x.size());

    for (size_t i = 0; i < out.size(); i++) {
      auto func = (clock.isEdge() ? GateSymbol::DFF : GateSymbol::LATCH);
      net.setGate(out[i], func, x[i] /* stored data */, clock.node());
    }
  } else {
    const auto &x = in[0];
    const auto &y = in[1];
    assert(x.size() == y.size() && out.size() == x.size());

    auto reset = invertIfNegative(control[1], net);

    for (size_t i = 0; i < out.size(); i++) {
      auto val = y[i]; // reset value
      auto neg = net.addNot(val);
      auto rst = net.addAnd(neg, reset.node());
      auto set = net.addAnd(val, reset.node());

      net.setDffrs(out[i], x[i] /* stored data */, clock.node(), rst, set);
    }
  }

  // Return the given outputs.
  return out;
}

FLibrary::Out FLibraryDefault::synthAdd(
    size_t outSize, const In &in, GNet &net) {
  return synthAdder(outSize, in, false, net);
}

FLibrary::Out FLibraryDefault::synthSub(
    size_t outSize, const In &in, GNet &net) {
  // The two's complement code: (x - y) == (x + ~y + 1).
  const auto &x = in[0];
  const auto &y = in[1];

  Out temp = synthUnaryBitwiseOp(GateSymbol::NOT, outSize, { y }, net);
  return synthAdder(outSize, { x, temp }, true, net);
}

FLibrary::Out FLibraryDefault::synthMul(
    size_t outSize, const In &in, GNet &net) {
  Out out(outSize);

  // TODO: Add multiplication implementation here.
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = net.addGate(GateSymbol::NOP, in[0][0]);
  }

  return out;
}

FLibrary::Out FLibraryDefault::synthAdder(
    size_t outSize, const In &in, bool plusOne, GNet &net) {
  assert(in.size() == 2);

  const auto &x = in[0];
  const auto &y = in[1];
  assert(x.size() == y.size() && outSize == x.size());

  auto carryIn = plusOne ? net.addOne() : net.addZero();

  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    const auto needsCarryOut = (i != out.size() - 1);
    auto zCarryOut = synthAdder(x[i], y[i], carryIn, needsCarryOut, net);

    out[i] = zCarryOut[0];
    carryIn = needsCarryOut ? zCarryOut[1] : Gate::INVALID;
  }

  return out;
}

FLibrary::Out FLibraryDefault::synthAdder(
    Gate::Id x, Gate::Id y, Gate::Id carryIn, bool needsCarryOut, GNet &net) {
  // {z, carryOut}.
  Out out;

  // z = (x + y) + carryIn (mod 2).
  auto xPlusY = net.addXor(x, y);
  out.push_back(net.addXor(xPlusY, carryIn));

  if (needsCarryOut) {
    // carryOut = (x & y) | (x + y) & carryIn.
    auto carryOutLhs = net.addAnd(x, y);
    auto carryOutRhs = net.addAnd(xPlusY, carryIn);
    out.push_back(net.addOr(carryOutLhs, carryOutRhs));
  }

  return out;
}

FLibrary::Out FLibraryDefault::synthMux(
    size_t outSize, const In &in, GNet &net) {
  assert(in.size() >= 4 && (in.size() & 1) == 0);
  const size_t n = in.size() / 2;

  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    std::vector<Signal> temp;
    temp.reserve(n);

    for (size_t j = 0; j < n; j++) {
      const GateIdList &c = in[j];
      const GateIdList &x = in[j + n];
      assert(c.size() == 1 && out.size() == x.size());

      auto id = net.addAnd(c[0], x[i]);
      temp.push_back(Signal::always(id));
    }

    out[i] = net.addGate(GateSymbol::OR, temp);
  }

  return out;
}

FLibrary::Out FLibraryDefault::synthUnaryBitwiseOp(
    GateSymbol func, size_t outSize, const In &in, GNet &net) {
  assert(in.size() == 1);

  const auto &x = in[0];
  assert(outSize == x.size());

  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = net.addGate(func, x[i]);
  }

  return out;
}

FLibrary::Out FLibraryDefault::synthBinaryBitwiseOp(
    GateSymbol func, size_t outSize, const In &in, GNet &net) {
  assert(in.size() == 2);

  const auto &x = in[0];
  const auto &y = in[1];
  assert(x.size() == y.size() && outSize == x.size());

  Out out(outSize);
  for (size_t i = 0; i < out.size(); i++) {
    out[i] = net.addGate(func, x[i], y[i]);
  }

  return out;
}

FLibrary::Signal FLibraryDefault::invertIfNegative(
    const Signal &event, GNet &net) {
  switch (event.event()) {
  case POSEDGE:
    // Leave the clock signal unchanged.
    return Signal::posedge(event.node());
  case NEGEDGE:
    // Invert the clock signal.
    return Signal::posedge(net.addNot(event.node()));
  case LEVEL0:
    // Invert the enable signal.
    return Signal::level1(net.addNot(event.node()));
  case LEVEL1:
    // Leave the enable signal unchanged.
    return Signal::level1(event.node());
  default:
    assert(false);
    return Signal::posedge(Gate::INVALID);
  }
}

} // namespace eda::rtl::library
