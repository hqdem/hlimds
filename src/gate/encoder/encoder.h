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
  void encode(const Netlist &net);
  void encode(const Gate &gate);

  void encode(const Context::Clause &clause) {
    _context.solver.addClause(clause);
  }

  Context& getContext() {
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

  void encodeFix(const Gate &gate, bool sign);
  void encodeBuf(const Gate &gate, bool sign);
  void encodeAnd(const Gate &gate, bool sign);
  void encodeOr (const Gate &gate, bool sign);
  void encodeXor(const Gate &gate, bool sign);

  /// Encodes the equality y^sign == x.
  void encodeBuf(uint64_t y, uint64_t x, bool sign);
  /// Encodes the equality y^sign == x1 ^ x2.
  void encodeXor(uint64_t y, uint64_t x1, uint64_t x2, bool sign);

private:
  Context _context;
};

} // namespace eda::gate::encoder
