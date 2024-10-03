//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/model/utils/subnetview_to_bdd.h"

namespace eda::gate::debugger {

using CellBddMap = model::utils::BddMap;

/// Checks the equivalence of the specified nets using BDD construction.
class BddChecker final : public BaseChecker,
                         public util::Singleton<BddChecker> {
  friend class util::Singleton<BddChecker>;

public:
  /// @copydoc BaseChecker::isSat
  CheckerResult isSat(const model::Subnet &subnet) const override;

private:
  BddChecker() {}
};

} // namespace eda::gate::debugger
