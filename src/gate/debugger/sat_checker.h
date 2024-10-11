//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/model/utils/subnet_cnf_encoder.h"

#include <cassert>
#include <vector>

namespace eda::gate::debugger {

class SatChecker final : public BaseChecker,
                         public util::Singleton<SatChecker> {
  friend class util::Singleton<SatChecker>;

public:
  /// @copydoc BaseChecker::isSat
  CheckerResult isSat(const model::Subnet &subnet) const override {
    assert(subnet.getOutNum() == 1 && "Miter w/ multiple outputs");

    solver::Solver solver;
    model::SubnetEncoderContext context(subnet, solver);

    const auto &encoder = model::SubnetEncoder::get();
    encoder.encode(subnet, context, solver);
    solver.addClause(context.lit(subnet.getOut(0), 1));

    if (solver.solve()) {
      std::vector<bool> counterExample;
      for (size_t i = 0; i < subnet.getInNum(); ++i) {
        counterExample.push_back(solver.value(context.var(i, 0)));
      }
      return CheckerResult(CheckerResult::NOTEQUAL, counterExample);
    }

    return CheckerResult::EQUAL;
  }

private:
  SatChecker() {}
};

} // namespace eda::gate::debugger
