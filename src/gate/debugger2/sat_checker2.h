//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger2/base_checker2.h"
#include "gate/debugger2/miter2.h"
#include "gate/model2/utils/subnet_cnf_encoder.h"

#include <cassert>

namespace eda::gate::debugger2 {

class SatChecker2 final : public BaseChecker2,
                          public util::Singleton<SatChecker2> {
  friend class util::Singleton<SatChecker2>;

public:
  CheckerResult equivalent(Subnet &lhs, Subnet &rhs, CellToCell &map) override {
    MiterHints hints = makeHints(lhs, map);
    Subnet miter = miter2(lhs, rhs, hints);

    const auto &encoder = SubnetEncoder::get();
    eda::gate::solver::Solver solver;
    SubnetEncoderContext context(miter, solver);

    encoder.encode(miter, context, solver);
    encoder.encodeEqual(miter, context, solver, miter.getOut(0), 1);

    return solver.solve() ? CheckerResult::NOTEQUAL : CheckerResult::EQUAL;
  }

private:
  SatChecker2() {}
};

} // namespace eda::gate::debugger2
