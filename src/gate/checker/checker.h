//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/netlist.h"

using namespace eda::gate::model;

namespace Minisat {
  class Solver;
} // namespace Minisat

namespace eda::gate::checker {

/**
 * \brief Implements a logic equivalence checker (LEC).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Checker final {
public:
  /// Checks logic equivalence of two netlists.
  bool check(const Netlist &lhs, const Netlist &rhs) const;

private:
  void encode(const Netlist &net, Minisat::Solver &solver);
  void encode(const Gate &gate, Minisat::Solver &solver);

  void encodeFix(const Gate &gate, bool sign, Minisat::Solver &solver);
  void encodeBuf(const Gate &gate, bool sign, Minisat::Solver &solver);
  void encodeAnd(const Gate &gate, bool sign, Minisat::Solver &solver);
  void encodeOr (const Gate &gate, bool sign, Minisat::Solver &solver);
  void encodeXor(const Gate &gate, bool sign, Minisat::Solver &solver);

  unsigned getNewVar();
};

} // namespace eda::gate::checker
