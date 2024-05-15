//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"

namespace eda::gate::debugger {

CheckerResult BddChecker::isSat(const SubnetID id) const {
  const Subnet &miter = Subnet::get(id);
  assert(miter.getOutNum() == 1);

  Cudd manager(0, 0);
  CellBddMap x;

  for (int i = 0; i < miter.getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }

  unsigned ouputId = miter.size() - 1;
  BDD netBdd = SubnetToBdd::convert(miter, ouputId, manager);

  if (netBdd == manager.bddZero()) {
    return CheckerResult::EQUAL;
  }
  return CheckerResult::NOTEQUAL;
}

} // namespace eda::gate::debugger
