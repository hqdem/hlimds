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

#include <unordered_map>

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
  AbcNpn4Synthesizer();

  /// Stores precomputed AIGs for practical NPN classes.
  model::SubnetBuilder builder;
  /// Maps NPN-canonical truth tables to the links in the builder.
  std::unordered_map<uint16_t, model::Subnet::Link> database;
};

} // namespace eda::gate::optimizer2
