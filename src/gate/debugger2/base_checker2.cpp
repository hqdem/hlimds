//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "base_checker2.h"
#include "bdd_checker2.h"
#include "rnd_checker2.h"
#include "sat_checker2.h"

namespace eda::gate::debugger2 {

using LecType = eda::gate::debugger2::options::LecType;

BaseChecker2 &getChecker(LecType lec) {
  switch (lec) {
    case LecType::BDD: return BddChecker2::get();
    case LecType::RND: return RndChecker2::get();
    case LecType::SAT: return SatChecker2::get();
    default: return SatChecker2::get();
  }
}
BaseChecker2::~BaseChecker2() {};

bool BaseChecker2::areEqual(const Subnet &lhs,
                            const Subnet &rhs,
                            const CellToCell &gmap) const {
  return equivalent(lhs, rhs, gmap).equal();
}

} // namespace eda::gate::debugger2
