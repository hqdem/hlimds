//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "base_checker2.h"
#include "rnd_checker2.h"

namespace eda::gate::debugger2 {

using LecType = eda::gate::debugger2::options::LecType;

BaseChecker2 &getChecker(LecType lec) {
  switch (lec) {
    default: return RndChecker2::get();
  }
}
BaseChecker2::~BaseChecker2() {};

bool BaseChecker2::areEqual(Subnet &lhs,
                            Subnet &rhs,
                            CellToCell &gmap) {
  return equivalent(lhs, rhs, gmap).equal();
}

} // namespace eda::gate::debugger2
