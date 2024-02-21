//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "base_checker2.h"
#include "gate/model2/utils/subnet_to_bdd.h"
#include "miter2.h"

namespace eda::gate::debugger2 {

using CellBddMap = model::utils::CellBDDMap;
using SubnetToBdd = model::utils::SubnetToBdd;

/// Checks the equivalence of the specified nets using BDD construction.
class BddChecker2 : public BaseChecker2, public util::Singleton<BddChecker2> {
friend class util::Singleton<BddChecker2>;

public:
  /**
   * @copydoc BaseChecker2::equivalent
   */
  CheckerResult equivalent(const Subnet &lhs,
                           const Subnet &rhs,
                           const CellToCell &gmap) const override;

private:
  BddChecker2() {}
};

} // namespace eda::gate::debugger2
