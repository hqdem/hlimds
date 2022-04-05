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
  /// Checks logic equivalence of two netlists.
  bool equiv(const Netlist &lhs,
             const Netlist &rhs,
	     const std::vector<std::pair<unsigned, unsigned>> &imap,
	     const std::vector<std::pair<unsigned, unsigned>> &omap) const;

private:
  void encode(unsigned offset, const Netlist &net,
              Minisat::Solver &solver) const;
  void encode(unsigned offset, const Gate &gate,
              Minisat::Solver &solver) const;

  void encodeFix(unsigned offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeBuf(unsigned offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeAnd(unsigned offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeOr (unsigned offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
  void encodeXor(unsigned offset, const Gate &gate, bool sign,
                 Minisat::Solver &solver) const;
};

} // namespace eda::gate::checker
