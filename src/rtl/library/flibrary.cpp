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

using namespace eda::gate::model;
using namespace eda::rtl::model;

namespace eda::rtl::library {

bool FLibraryDefault::supports(FuncSymbol func) const {
  return true;
}

void FLibraryDefault::synth(const Out &out, const Value &value, GNet &net) {
  assert(out.size() == value.size());

  for (std::size_t i = 0; i < out.size(); i++) {
    net.setGate(out[i], (value[i] ? GateSymbol::ONE : GateSymbol::ZERO), {});
  }
}

void FLibraryDefault::synth(FuncSymbol func,
                            const Out &out,
                            const In &in,
                            GNet &net) {
  switch (func) {
  case FuncSymbol::NOP:
    synthUnaryBitwiseOp<GateSymbol::NOP>(out, in, net);
    break;
  case FuncSymbol::NOT:
    synthUnaryBitwiseOp<GateSymbol::NOT>(out, in, net);
    break;
  case FuncSymbol::AND:
    synthBinaryBitwiseOp<GateSymbol::AND>(out, in, net);
    break;
  case FuncSymbol::OR:
    synthBinaryBitwiseOp<GateSymbol::OR>(out, in, net);
    break;
  case FuncSymbol::XOR:
    synthBinaryBitwiseOp<GateSymbol::XOR>(out, in, net);
    break;
  case FuncSymbol::ADD:
    synthAdd(out, in, net);
    break;
  case FuncSymbol::SUB:
    synthSub(out, in, net);
    break;
  case FuncSymbol::MUX:
    synthMux(out, in, net);
    break;
  default:
    assert(false);
  }
}

void FLibraryDefault::synth(const Out &out,
                            const In &in,
                            const Signal::List &control,
                            GNet &net) {
  assert(control.size() == 1 || control.size() == 2);
  assert(control.size() == in.size());

  auto clock = invertIfNegative(control[0], net);
  if (control.size() == 1) {
    for (std::size_t i = 0; i < out.size(); i++) {
      auto f = (clock.edge() ? GateSymbol::DFF : GateSymbol::LATCH);
      auto d = Signal::always(in[0][i]); // stored data

      net.setGate(out[i], f, { d, clock });
    }
  } else {
    auto edged = invertIfNegative(control[1], net);
    auto reset = Signal::always(edged.gateId());

    for (std::size_t i = 0; i < out.size(); i++) {
      auto d = Signal::always(in[0][i]); // stored data
      auto v = Signal::always(in[1][i]); // reset value
      auto n = Signal::always(net.addGate(GateSymbol::NOT, { v }));
      auto r = Signal::level1(net.addGate(GateSymbol::AND, { n, reset }));
      auto s = Signal::level1(net.addGate(GateSymbol::AND, { v, reset }));

      net.setGate(out[i], GateSymbol::DFFrs, { d, clock, r, s });
    }
  }
}

void FLibraryDefault::synthAdd(const Out &out, const In &in, GNet &net) {
  synthAdder(out, in, false, net);
}

void FLibraryDefault::synthSub(const Out &out, const In &in, GNet &net) {
  // The two's complement code: (x - y) == (x + ~y + 1).
  const auto &x = in[0];
  const auto &y = in[1];

  Out temp(y.size());
  for (std::size_t i = 0; i < y.size(); i++) {
    temp[i] = net.newGate();
  }

  synthUnaryBitwiseOp<GateSymbol::NOT>(temp, { y }, net);
  synthAdder(out, { x, temp }, true, net);
}

void FLibraryDefault::synthAdder(const Out &out, const In &in, bool plusOne, GNet &net) {
  assert(in.size() == 2);

  const auto &x = in[0];
  const auto &y = in[1];
  assert(x.size() == y.size() && out.size() == x.size());

  auto carryIn = Gate::INVALID;
  auto carryOut = net.addGate(plusOne ? GateSymbol::ONE : GateSymbol::ZERO, {});

  for (std::size_t i = 0; i < out.size(); i++) {
    carryIn = carryOut;
    carryOut = (i != out.size() - 1 ? net.newGate() : Gate::INVALID);
    synthAdder(out[i], carryOut, x[i], y[i], carryIn, net);
  }
}

void FLibraryDefault::synthAdder(Gate::Id z,
                                 Gate::Id carryOut,
                                 Gate::Id x,
                                 Gate::Id y,
                                 Gate::Id carryIn,
                                 GNet &net) {
  auto xWire = Signal::always(x);
  auto yWire = Signal::always(y);
  auto cWire = Signal::always(carryIn);

  // z = (x + y) + carryIn (mod 2).
  auto xPlusY = Signal::always(net.addGate(GateSymbol::XOR, { xWire, yWire }));
  net.setGate(z, GateSymbol::XOR, { xPlusY, cWire });

  if (carryOut != Gate::INVALID) {
    // carryOut = (x & y) | (x + y) & carryIn.
    auto carryOutLhs = Signal::always(net.addGate(GateSymbol::AND,
                                                  { xWire, yWire }));
    auto carryOutRhs = Signal::always(net.addGate(GateSymbol::AND,
                                                  { xPlusY, cWire }));
    net.setGate(carryOut, GateSymbol::OR, { carryOutLhs, carryOutRhs });
  }
}

void FLibraryDefault::synthMux(const Out &out, const In &in, GNet &net) {
  assert(in.size() >= 4 && (in.size() & 1) == 0);
  const std::size_t n = in.size() / 2;

  for (std::size_t i = 0; i < out.size(); i++) {
    std::vector<Signal> temp;
    temp.reserve(n);

    for (std::size_t j = 0; j < n; j++) {
      const GateIdList &c = in[j];
      const GateIdList &x = in[j + n];
      assert(c.size() == 1 && out.size() == x.size());

      auto cj0 = Signal::always(c[0]);
      auto xji = Signal::always(x[i]);
      auto id = net.addGate(GateSymbol::AND, { cj0, xji });

      temp.push_back(Signal::always(id));
    }

    net.setGate(out[i], GateSymbol::OR, temp);
  }
}

Signal FLibraryDefault::invertIfNegative(const Signal &event, GNet &net) {
  switch (event.kind()) {
  case Event::POSEDGE:
    // Leave the clock signal unchanged.
    return Signal::posedge(event.gateId());
  case Event::NEGEDGE:
    // Invert the clock signal.
    return Signal::posedge(net.addGate(GateSymbol::NOT,
                                       { Signal::always(event.gateId()) }));
  case Event::LEVEL0:
    // Invert the enable signal.
    return Signal::level1(net.addGate(GateSymbol::NOT,
                                      { Signal::always(event.gateId()) }));
  case Event::LEVEL1:
    // Leave the enable signal unchanged.
    return Signal::level1(event.gateId());
  default:
    assert(false);
    return Signal::posedge(Gate::INVALID);
  }
}

} // namespace eda::rtl::library
