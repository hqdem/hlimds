//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/encoder.h"
#include "gate/model/gnet.h"

#include <vector>

using namespace eda::gate::model;

namespace eda::gate::debugger {

/**
 * \brief Represents hints for logic equivalence checking (LEC).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
struct Hints {
  // TODO:
};

/**
 * \brief Implements a logic equivalence checker (LEC).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Checker final {
public:
  using GateIdList = GNet::GateIdList;
  using GateBind = std::pair<Gate::Id, Gate::Id>;
  using GateBindList = std::vector<GateBind>;
  using GateIdMap = Context::GateIdMap;

  /// Checks logic equivalence of two nets.
  bool areEqual(const GNet &lhs,
                const GNet &rhs,
                const GateBindList &ibind,
                const GateBindList &obind,
                const Hints &hints) const;

  /// Checks logic equivalence of two flat combinational nets.
  bool areEqualComb(const GNet &lhs,
                    const GNet &rhs,
	            const GateBindList &ibind,
	            const GateBindList &obind) const;

  /// Checks logic equivalence of two flat sequential nets
  /// with one-to-one correspondence of triggers.
  bool areEqualSeq(const GNet &lhs,
                   const GNet &rhs,
                   const GateBindList &ibind,
                   const GateBindList &obind,
                   const GateBindList &tbind) const;

  /// Checks logic equivalence of two flat sequential nets
  /// with given correspondence of state encodings.
  bool areEqualSeq(const GNet &lhs,
                   const GNet &rhs,
                   const GNet &enc,
                   const GNet &dec,
                   const GateBindList &ibind,
                   const GateBindList &obind,
                   const GateBindList &lhsTriEncIn,
                   const GateBindList &lhsTriDecOut,
                   const GateBindList &rhsTriEncOut,
                   const GateBindList &rhsTriDecIn) const;

  /// Checks isomorphism of two nets.
  bool areIsomorphic(const GNet &lhs,
                     const GNet &rhs,
                     const GateBindList &ibind,
                     const GateBindList &obind) const;

private:
  /// Checks logic equivalence of two flat combinational nets.
  bool areEqualComb(const std::vector<const GNet*> &nets,
                    const GateIdMap *connectTo,
	            const GateBindList &ibind,
	            const GateBindList &obind) const;

  /// Handles an error (prints the diagnostics, etc.).
  void error(Context &context,
	     const GateBindList &ibind,
	     const GateBindList &obind) const;
};

} // namespace eda::gate::debugger
