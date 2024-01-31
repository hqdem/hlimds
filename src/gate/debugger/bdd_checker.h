//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "base_checker.h"
#include "gate/debugger/miter.h"
#include "gate/transformer/bdd.h"

namespace eda::gate::debugger {

using BddList = transformer::GNetBDDConverter::BDDList;
using GateBDDMap = transformer::GNetBDDConverter::GateBDDMap;
using GateId = model::Gate::Id;
using GateSymbol = model::GateSymbol;
using GNet = eda::gate::model::GNet;
using GNetBDDConverter = transformer::GNetBDDConverter;
using Signal = model::Gate::Signal;
using SignalList = model::Gate::SignalList;

/// Checks the equivalence of the specified nets using BDD construction.
class BddChecker : public BaseChecker, public util::Singleton<BddChecker> {
friend class util::Singleton<BddChecker>;

public:
  /**
   * @copydoc BaseChecker::equivalent
   */
  CheckerResult equivalent(const GNet &lhs,
                           const GNet &rhs,
                           const SatChecker::GateIdMap &gmap) const override;
private:
  BddChecker() {}

};

} // namespace eda::gate::debugger
