//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/synthesizer.h"
#include "util/singleton.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace eda::gate::optimizer2 {


/**
 * \brief Searches for an implementation of a 4-variable function in the database
 *        of precomputed AIGs for practical NPN classes (as it is done in ABC).
 */
class AbcNpn4Synthesizer final : public TruthTableSynthesizer,
                                 public util::Singleton<AbcNpn4Synthesizer> {
  friend class util::Singleton<AbcNpn4Synthesizer>;

public:
  using TruthTable = FunctionIR;

  SubnetID synthesize(const TruthTable &tt, uint16_t maxArity = -1) override;

private:
  struct Database final {
    static constexpr auto k = 4;

    struct Node final {
      Node():
          table(4), symbol(model::ZERO) {}

      Node(TruthTable table,
           model::CellSymbol symbol):
          table(table), symbol(symbol) {}

      Node(TruthTable table,
           model::CellSymbol symbol,
           model::Subnet::Link link0,
           model::Subnet::Link link1):
          table(table), symbol(symbol), link{link0, link1} {}

      const TruthTable table;
      const model::CellSymbol symbol;
      const model::Subnet::Link link[2];
    };

    Database();

    /// Returns the subnetID for the given table.
    model::SubnetID find(const TruthTable &tt) const;

    /// Stores precomputed AIGs for practical NPN classes.
    std::vector<Node> aig;
    /// Maps NPN-canonical truth tables to the links in the builder.
    std::unordered_map<uint16_t, size_t> map;
  };

  AbcNpn4Synthesizer(): cache(1 << (1 << 4)), database() {}

  /// Stores synthesized subnets (index = truth table).
  std::vector<SubnetID> cache;
  /// Stores AIGs of frequent 4-variable functions.
  Database database;
};

} // namespace eda::gate::optimizer2
