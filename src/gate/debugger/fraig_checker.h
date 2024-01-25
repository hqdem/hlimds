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
#include "gate/optimizer/util.h"
#include "gate/premapper/aigmapper.h"
#include "gate/simulator/simulator.h"

#include <random>

namespace eda::gate::debugger {

using BoolVector = std::vector<bool>;
using BoundGNet = optimizer::BoundGNet;
using Compiled = simulator::Simulator::Compiled;
using EqClasses = std::vector<std::vector<GateId>>;
using Gate = model::Gate;
using GateId = model::Gate::Id;
using GateIdMap = SatChecker::GateIdMap;
using GateSymbol = model::GateSymbol;
using GidPair = std::pair<GateId, GateId>;
using GNet = eda::gate::model::GNet;
using Hints = SatChecker::Hints;

/**
 * \brief Implements FRAIG-based method of LEC.
 *
 * The algorithm is based on the article "Improvements to combinational
 * equivalence checking" by A. Mishchenko, S. Chatterjee, R. Brayton (2006).
 */
class FraigChecker : public BaseChecker, public util::Singleton<FraigChecker> {
friend class util::Singleton<FraigChecker>;

public:
  /**
   * @copydoc BaseChecker::equivalent
   */
  CheckerResult equivalent(const GNet &lhs,
                           const GNet &rhs,
                           const GateIdMap &gmap) const override;
  // TODO Figure out proper time/values of initial/subsequent simulations.
  /// Number of iterations of the initial simulation.
  static const size_t initSimNum = 32;
  /// Number of iterations during one round of the saturation simulation.
  static const size_t saturSimNum = 5;

  // TODO Figure out suitable default values
  /// Cones comparison time limit, number of iterations
  static const size_t tLimit = 1000;
  /// Maximum number of inputs and outputs the simulator can handle.
  static const size_t simLimit = 64;
private:
  static EqClasses initClasses(GNet *premapped);

  static void mergeConsts(std::vector<GateId> &consts,
                          GNet *premapped,
                          bool oneOrZero);

  static void splitClass(EqClasses &eqClasses,
                         std::vector<GateId> &eqClass,
                         Compiled &comp);

  static void saturation(Compiled &compiled,
                         EqClasses &classes,
                         GNet &premapped,
                         std::vector<BoolVector> simValues = {});
  FraigChecker () {}
};

} // namespace eda::gate::debugger

