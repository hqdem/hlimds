//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesizer.h"
#include "util/singleton.h"

#include <unordered_map>
#include <vector>

namespace eda::gate::optimizer {

/**
 * \brief Searches for an implementation of a 4-variable function in the database
 *        of precomputed AIGs for practical NPN classes (as it is done in ABC).
 */
class AbcNpn4Synthesizer final : public TruthTableSynthesizer,
                                 public util::Singleton<AbcNpn4Synthesizer> {
  friend class util::Singleton<AbcNpn4Synthesizer>;

public:
  using TruthTable = FunctionIR;
  static constexpr auto k = 4;

  using Synthesizer::synthesize;

  model::SubnetID synthesize(
      const TruthTable &tt,
      const TruthTable &,
      const uint16_t maxArity = -1) const override;

#ifdef NPN4_USAGE_STATS
  void printNpn4UsageStats();
#endif // NPN4_USAGE_STATS

private:
  AbcNpn4Synthesizer() {}
};

} // namespace eda::gate::optimizer
