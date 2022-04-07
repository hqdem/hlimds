//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/encoder/context.h"
#include "gate/model/netlist.h"

#include "minisat/core/Solver.h"

#include <vector>

using namespace eda::gate::model;

namespace eda::gate::encoder {

/**
 * \brief Implements a Tseitin encoder of a gate-level netlist.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Encoder final {
public:
  void encode(const Netlist &net, uint16_t version);
  void encode(const Gate &gate, uint16_t version);

  void encode(const Context::Clause &clause) {
    _context.solver.addClause(clause);
  }

  Context& context() {
    return _context;
  }

  void setOffset(std::size_t offset) {
    _context.offset = offset;
  }

  uint64_t newVar() {
    return _context.newVar();
  }

  bool solve() {
    return _context.solver.solve();
  }

  // Combinational gates.
  void encodeFix(const Gate &gate, bool sign, uint16_t version);
  void encodeBuf(const Gate &gate, bool sign, uint16_t version);
  void encodeAnd(const Gate &gate, bool sign, uint16_t version);
  void encodeOr (const Gate &gate, bool sign, uint16_t version);
  void encodeXor(const Gate &gate, bool sign, uint16_t version);

  // Latches and flip-flops.
  void encodeLatch(const Gate &gate, uint16_t version);
  void encodeDff  (const Gate &gate, uint16_t version);
  void encodeDffRs(const Gate &gate, uint16_t version);

  /// Encodes the equality y^s == x.
  void encodeBuf(uint64_t y, uint64_t x, bool s);
  /// Encodes the equality y^s == x1^s1 | x2^s2.
  void encodeAnd(uint64_t y, uint64_t x1, uint64_t x2, bool s, bool s1, bool s2);
  /// Encodes the equality y^s == x1^s1 | x2^s2.
  void encodeOr (uint64_t y, uint64_t x1, uint64_t x2, bool s, bool s1, bool s2);
  /// Encodes the equality y^s == x1^s1 ^ x2^s2.
  void encodeXor(uint64_t y, uint64_t x1, uint64_t x2, bool s, bool s1, bool s2);
  /// Encodes the equality y^s == c ? x1 : x2.
  void encodeMux(uint64_t y, uint64_t c, uint64_t x1, uint64_t x2, bool s);

private:
  Context _context;
};

} // namespace eda::gate::encoder
