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

#include <cstdint>

using namespace eda::gate::model;

namespace eda::gate::encoder {

/**
 * \brief Logic formula representing a gate-level netlist.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
struct Context final {
  // MiniSAT-related types.
  using Var = Minisat::Var;
  using Lit = Minisat::Lit;
  using Clause = Minisat::vec<Lit>;

  // Signal access mode.
  enum Mode { GET, SET };

  /**
   * Returns a variable id, which is an integer of the following format:
   *
   * |0..0|Version|GateId|New|
   *    2     8      20   (1)  32 bits [as is]
   *  (16)  (16)    (31)  (1)  64 bits [to be]
   *
   * The version is used for symbolic execution and can borrow bits for id.
   * The current limitations on the field widths are caused by MiniSAT.
   */
  static uint64_t var(std::size_t offset, unsigned gateId, uint16_t version) {
    return ((uint64_t)version << 21) |
           ((uint64_t)offset + gateId) << 1;
  }

  /// Creates a literal.
  static Lit lit(uint64_t var, bool sign) {
    return Minisat::mkLit(static_cast<Var>(var), sign);
  }

  /// Returns a variable id.
  uint64_t var(unsigned gateId, uint16_t version) {
    return var(offset, gateId, version);
  }

  /// Returns a variable id.
  uint64_t var(const Gate &gate, uint16_t version, Mode mode) {
    return (mode == GET && gate.is_trigger() && version > 0)
      ? var(offset, gate.id(), version - 1)
      : var(offset, gate.id(), version);
  }

  /// Returns a variable id.
  uint64_t var(const Signal &signal, uint16_t version, Mode mode) {
    return var(*signal.gate(), version, mode);
  }

  /// Returns a new variable id.
  uint64_t newVar() {
    // See the variable id format.
    static uint64_t var = 0;
    return ((var++) << 1) | 1;
  }

  /// Dumps the current formula to the file.
  void dump(const std::string &file) {
    solver.toDimacs(file.c_str());
  }

  std::size_t offset;
  Minisat::Solver solver;
};

} // namespace eda::gate::encoder
