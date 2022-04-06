//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/netlist.h"

#include "minisat/core/Solver.h"

using namespace eda::gate::model;

namespace eda::gate::encoder {

/**
 * \brief Logic formula representing a gate-level netlist.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
struct Context final {
  using Var = Minisat::Var;
  using Lit = Minisat::Lit;
  using Clause = Minisat::vec<Lit>;

  /// Returns a variable id.
  static unsigned var(std::size_t offset, unsigned gateId) {
    return offset + gateId;
  }

  /// Creates a literal.
  static Lit lit(unsigned var, bool sign) {
    return Minisat::mkLit(static_cast<Var>(var), sign);
  }

  /// Returns a variable id.
  unsigned var(unsigned gateId) {
    return var(offset, gateId);
  }

  /// Returns a variable id.
  unsigned var(const Gate &gate) {
    return var(offset, gate.id());
  }

  /// Returns a variable id.
  unsigned var(const Signal &signal) {
    return var(*signal.gate());
  }

  /// Returns a new variable id.
  unsigned newVar() {
    static unsigned var = 1024*1024;
    return var++;
  }

  std::size_t offset;
  Minisat::Solver solver;
};

} // namespace eda::gate::encoder
