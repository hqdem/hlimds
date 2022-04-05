//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/netlist.h"

#include <vector>

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
  using GateBind = std::pair<unsigned, unsigned>;
  using GateBindList = std::vector<GateBind>;

  /// Checks logic equivalence of two combinational netlists.
  bool equiv(const Netlist &lhs,
             const Netlist &rhs,
	     const GateBindList &ibind,
	     const GateBindList &obind) const;

  /// Checks logic equivalence of two sequential netlists
  /// with one-to-one correspondence of triggers.
  bool equiv(const Netlist &lhs,
             const Netlist &rhs,
             const GateBindList &ibind,
             const GateBindList &obind,
             const GateBindList &tbind) const;
	     
private:
  void encode(std::size_t offset, const Netlist &net,
              Minisat::Solver &solver) const;
  void encode(std::size_t offset, const Gate &gate,
              Minisat::Solver &solver) const;

  void encodeFix(std::size_t offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeBuf(std::size_t offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeAnd(std::size_t offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeOr (std::size_t offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeXor(std::size_t offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
};

} // namespace eda::gate::checker
