//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/encoder/encoder.h"

#include <cassert>

namespace eda::gate::encoder {

void Encoder::encode(const Netlist &net) {
  for (const auto *gate: net.gates()) {
    encode(*gate);
  }
}

void Encoder::encode(const Gate &gate) {
  if (gate.is_source())
    return;

  switch (gate.kind()) {
  case GateSymbol::ONE:
    encodeFix(gate, true);
    break;
  case GateSymbol::ZERO:
    encodeFix(gate, false);
    break;
  case GateSymbol::NOP:
    encodeBuf(gate, true);
    break;
  case GateSymbol::NOT:
    encodeBuf(gate, false);
    break;
  case GateSymbol::AND:
    encodeAnd(gate, true);
    break;
   case GateSymbol::NAND:
    encodeAnd(gate, false);
    break;
  case GateSymbol::OR:
    encodeOr(gate, true);
    break;
  case GateSymbol::NOR:
    encodeOr(gate, false);
    break;
  case GateSymbol::XOR:
    encodeXor(gate, true);
    break;
  case GateSymbol::XNOR:
    encodeXor(gate, false);
    break;
  default:
    assert(false && "Unsupported gate");
  }
}

void Encoder::encodeFix(const Gate &gate, bool sign) {
  const unsigned x = _context.var(gate);
  _context.solver.addClause(Context::lit(x, sign));
}

void Encoder::encodeBuf(const Gate &gate, bool sign) {
  const unsigned x = _context.var(gate.input(0));
  const unsigned y = _context.var(gate);

  encodeBuf(y, x, sign);
}

void Encoder::encodeAnd(const Gate &gate, bool sign) {
  const unsigned y = _context.var(gate);
  Context::Clause clause(gate.arity() + 1);
  
  clause.push(Context::lit(y, sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = _context.var(input);

    clause.push(Context::lit(x, false));
    _context.solver.addClause(Context::lit(y, !sign),
                              Context::lit(x, true));
  }

  _context.solver.addClause(clause);
}

void Encoder::encodeOr(const Gate &gate, bool sign) {
  const unsigned y = _context.var(gate);
  Context::Clause clause(gate.arity() + 1);
  
  clause.push(Context::lit(y, !sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = _context.var(input);

    clause.push(Context::lit(x, true));
    _context.solver.addClause(Context::lit(y, sign),
                              Context::lit(x, false));
  }

  _context.solver.addClause(clause);
}

void Encoder::encodeXor(const Gate &gate, bool sign) {
  if (gate.arity() == 1) {
    return encodeBuf(gate, sign);
  }

  unsigned y = _context.var(gate);
  for (unsigned i = 0; i < gate.arity() - 1; i++) {
    const unsigned x1 = _context.var(gate.input(i));
    const unsigned x2 = (i == gate.arity() - 2)
      ? _context.var(gate.input(i + 1))
      : _context.newVar();

    encodeXor(y, x1, x2, sign);
    y = x2;
  }
}

void Encoder::encodeBuf(unsigned y, unsigned x, bool sign) {
  _context.solver.addClause(Context::lit(x, true),
                            Context::lit(y, !sign));
  _context.solver.addClause(Context::lit(x, false),
                            Context::lit(y, sign));
}

void Encoder::encodeXor(unsigned y, unsigned x1, unsigned x2, bool sign) {
  _context.solver.addClause(Context::lit(y,  sign),
                            Context::lit(x1, true),
                            Context::lit(x2, true));
  _context.solver.addClause(Context::lit(y,  sign),
                            Context::lit(x1, false),
                            Context::lit(x2, false));
  _context.solver.addClause(Context::lit(y, !sign),
                            Context::lit(x1, true),
                            Context::lit(x2, false));
  _context.solver.addClause(Context::lit(y, !sign),
                            Context::lit(x1, false),
                            Context::lit(x2, true));
}

} // namespace eda::gate::encoder
