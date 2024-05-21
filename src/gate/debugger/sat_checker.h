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

namespace eda::gate::debugger {

class SatChecker final : public BaseChecker,
                         public util::Singleton<SatChecker> {
  friend class util::Singleton<SatChecker>;

public:
  CheckerResult isSat(const model::Subnet &subnet) const override {
    assert(subnet.getOutNum() == 1);

    const auto &encoder = model::SubnetEncoder::get();
    solver::Solver solver;
    model::SubnetEncoderContext context(subnet, solver);

    encoder.encode(subnet, context, solver);
    encoder.encodeEqual(subnet, context, solver, subnet.getOut(0), 1);

    if (solver.solve()) {
      std::vector<bool> counterEx;
      for (size_t i = 0; i < subnet.getInNum(); ++i) {
        counterEx.push_back(solver.value(context.var(i, 0)));
      }
      return CheckerResult(CheckerResult::NOTEQUAL, counterEx);
    }

    return CheckerResult::EQUAL;
  }

private:
  SatChecker() {}
};

} // namespace eda::gate::debugger
