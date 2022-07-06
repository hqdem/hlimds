//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"

namespace eda::gate::simulator {

using Compiled = Simulator::Compiled;

Compiled::OP Compiled::getOp(const Gate &gate) const {
  const auto n = gate.arity();

  switch (gate.kind()) {
  case  ZERO: return getZero(n);
  case   ONE: return getOne(n);
  case   NOP: return getNop(n);
  case   NOT: return getNot(n);
  case   AND: return getAnd(n);
  case    OR: return getOr(n);
  case   XOR: return getXor(n);
  case  NAND: return getNand(n);
  case   NOR: return getNor(n);
  case  XNOR: return getXnor(n);
  case LATCH: return getLatch(n);
  case   DFF: return getDff(n);
  case DFFrs: return getDffrs(n);
  default: assert(false);
  }

  return getZero(0);
}

Compiled::Command Compiled::getCommand(const GNet &net,
                                       const Gate &gate) const {
  const auto target = gate.id();
  Gate::Link outLink(target);

  OP op = getOp(gate);
  I out = gindex.find(outLink)->second;
  IV in(gate.arity());

  for (I i = 0; i < gate.arity(); i++) {
    const auto source = gate.input(i).gateId();

    Gate::Link inLink = net.contains(source)
                      ? Gate::Link(source)
                      : Gate::Link(source, target, i);

    in[i] = gindex.find(inLink)->second;
  }

  return Command{op, out, in};
}

Compiled::Compiled(const GNet &net, const GNet::GateIdList &out):
    program(net.nGates()),
    nInputs(net.nSourceLinks()),
    outputs(out.size()),
    memory(net.nSourceLinks() + net.nGates()),
    postponed(net.nTriggers()),
    nPostponed(0) {

  assert(net.isSorted() && "Net is not topologically sorted");
  gindex.reserve(net.nSourceLinks() + net.nGates());

  I i = 0;
  for (const auto link : net.sourceLinks()) {
    gindex[link] = i++;
  }

  for (const auto *gate : net.gates()) {
    if (!gate->isSource()) {
      Gate::Link link(gate->id());
      gindex[link] = i++;
    }
  }

  i = 0;
  for (const auto outId : out) {
    Gate::Link link(outId);
    outputs[i++] = gindex[link];
  }

  i = 0;
  for (const auto *gate : net.gates()) {
    program[i++] = getCommand(net, *gate);
  }
}

Compiled::BV Compiled::simulate(const Compiled::BV &value) {
  // Apply the postponed assignments.
  while (nPostponed > 0) {
    const auto assign = postponed[--nPostponed];
    const auto lhs = assign.first;
    const auto rhs = assign.second;

    memory[lhs] = rhs;
  }

  // Assign the input values.
  assert(value.size() == nInputs);
  for (std::size_t i = 0; i < value.size(); i++) {
    memory[i] = value[i];
  }

  // Execute the commands.
  for (const auto &command : program) {
    command.op(command.out, command.in);
  }

  // Extract the output values.
  BV result(outputs.size());
  for (std::size_t i = 0; i < outputs.size(); i++) {
    result[i] = memory[outputs[i]];
  }

  return result;
}

} // namespace eda::gate::simulator
