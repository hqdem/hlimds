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

namespace eda::gate::checker {

/**
 * \brief Implements a logic equivalence checker (LEC).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Checker final {
public:
  using GateIdList = Netlist::GateIdList;
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

  /// Checks logic equivalence of two sequential netlists
  /// with given correspondence of state encodings.
  bool equiv(const Netlist &lhs,
             const Netlist &rhs,
             const Netlist &enc,
             const Netlist &dec,
             const GateBindList &ibind,
             const GateBindList &obind,
             const GateBindList &encOutDecIn,
             const GateBindList &lhsTriEncIn,
             const GateBindList &lhsTriDecOut) const;
};

} // namespace eda::gate::checker
