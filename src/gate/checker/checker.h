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

class Solver;

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
  void encode(const Netlist &net, Solver &solver);
  void encode(const Gate &gate, Solver &solver);
  void encodeZero(const Gate &gate, Solver &solver);
  void encodeOne(const Gate &gate, Solver &solver);
  void encodeNop(const Gate &gate, Solver &solver);
  void encodeNot(const Gate &gate, Solver &solver);
  void encodeAnd(const Gate &gate, Solver &solver);
  void encodeOr(const Gate &gate, Solver &solver);
  void encodeXor(const Gate &gate, Solver &solver);
  void encodeNand(const Gate &gate, Solver &solver);
  void encodeNor(const Gate &gate, Solver &solver);
  void encodeXnor(const Gate &gate, Solver &solver);
};

} // namespace eda::gate::checker
