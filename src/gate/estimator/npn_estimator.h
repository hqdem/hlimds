//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/estimator.h"
#include "gate/function/truth_table.h"
#include "gate/model/subnet.h"
#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"

#include <unordered_map>

namespace eda::gate::estimator {

using NpnStats = std::unordered_map<kitty::dynamic_truth_table, size_t>;

struct NpnSettings;

class NpnEstimator : public SubnetEstimator<NpnSettings, NpnStats> {
public:

  using SubnetBuilderPtr = std::shared_ptr<gate::model::SubnetBuilder>;

  void estimate(const SubnetBuilderPtr &builder,
                const NpnSettings &settings,
                NpnStats &result) const override;
};

struct NpnSettings {
public:
  NpnSettings(uint16_t k, bool extendTables, bool countTrivial) :
      k(k), extendTables(extendTables), countTrivial(countTrivial) {}
  NpnSettings(uint16_t k) : NpnSettings(k, true, false) {}
  NpnSettings() : NpnSettings(4) {}

  /// The cut size for NPN-classes estimation.
  uint16_t k;
  /// Enables to count tables with arity less than k.
  bool extendTables;
  /// Enables to count trivial cuts.
  bool countTrivial;
};

} // namespace eda::gate::estimator
