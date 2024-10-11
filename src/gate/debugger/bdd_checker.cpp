//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"
#include "gate/model/utils/subnetview_to_bdd.h"

#include <cassert>

namespace eda::gate::debugger {

CheckerResult BddChecker::isSat(const model::Subnet &subnet) const {
  assert(subnet.getOutNum() == 1);

  Cudd manager(0, 0);

  auto builderPtr = std::make_shared<model::SubnetBuilder>(subnet);
  model::SubnetView sv(builderPtr);
  BDD netBdd = model::utils::convertBdd(sv, manager).at(0);

  return (netBdd == manager.bddZero())
      ? CheckerResult::EQUAL
      : CheckerResult::NOTEQUAL;
}

} // namespace eda::gate::debugger
