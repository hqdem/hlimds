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
#include <unordered_map>

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
  using Solver = Minisat::Solver;

  // Gate reconnection map.
  using GateIdMap = std::unordered_map<unsigned, unsigned>;

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
  static uint64_t var(const GateIdMap *connectTo,
                      unsigned gateId,
                      uint16_t version) {
    // FIXME: Such encoding is not suitable for MiniSAT w/ IntMap implemented as a vector.
    return ((uint64_t)version << 21) |
           ((uint64_t)connectedTo(connectTo, gateId) << 1);
  }

  /// Returns the gate id the given one is connected to.
  static unsigned connectedTo(const GateIdMap *connectTo, unsigned gateId) {
    if (connectTo) {
      auto i = connectTo->find(gateId);
      if (i != connectTo->end())
        return i->second;
    }

    return gateId;
  }

  /// Creates a literal.
  static Lit lit(uint64_t var, bool sign) {
    const Lit literal = Minisat::mkLit(static_cast<Var>(var));
    return sign ? literal : ~literal;
  }

  /// Returns a variable id.
  uint64_t var(unsigned gateId, uint16_t version) {
    return var(connectTo, gateId, version);
  }

  /// Returns a variable id.
  uint64_t var(const Gate &gate, uint16_t version, Mode mode) {
    return (mode == GET && gate.is_trigger() && version > 0)
      ? var(gate.id(), version - 1)
      : var(gate.id(), version);
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

  /// Reserves the variable in the solver.
  void reserve(uint64_t var) {
    while (var >= (uint64_t)solver.nVars()) {
      solver.newVar();
    }
  }

  const GateIdMap *connectTo = nullptr;
  Solver solver;
};

} // namespace eda::gate::encoder
