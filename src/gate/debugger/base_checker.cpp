//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "base_checker.h"
#include "bdd_checker.h"
#include "fraig_checker.h"
#include "rnd_checker.h"
#include "sat_checker.h"

namespace eda::gate::debugger {

using LecType = eda::gate::debugger::options::LecType;

BaseChecker &getChecker(LecType lec) {
  switch (lec) {
    case LecType::BDD: return BddChecker::get();
    case LecType::FRAIG: return FraigChecker::get();
    case LecType::RND: return RndChecker::get();
    case LecType::SAT: return SatChecker::get();
    default: return SatChecker::get();
  }
}
BaseChecker::~BaseChecker() {};

bool BaseChecker::areEqual(const GNet &lhs,
                           const GNet &rhs,
                           const GateIdMap &gmap) {
  return equivalent(lhs, rhs, gmap).equal();
}

} // namespace eda::gate::debugger
