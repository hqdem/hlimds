//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/area_resynthesizer.h"
#include "gate/optimizer/synthesis/akers.h"
#include "gate/optimizer/synthesis/isop.h"

#include "kitty/kitty.hpp"

namespace eda::gate::optimizer {

using SubnetID = eda::gate::model::SubnetID;

SubnetID AreaResynthesizer::resynthesize(SubnetID subnetId,
                                         const TruthTable &care,
                                         uint16_t maxArity) const {

  // Heuristic constants for optimizers calling.
  const size_t akersMaxInputNum = 8;
  const size_t akersArityNum    = 3;

  const auto &subnet = Subnet::get(subnetId);
  auto truthTable = gate::model::evaluateSingleOut(subnet);
  const auto numVars = truthTable.num_vars();

  SubnetID resynthesizedId = model::OBJ_NULL_ID;
  if ((numVars < akersMaxInputNum) && (maxArity == akersArityNum)) {
    synthesis::AkersSynthesizer akers;
    resynthesizedId = akers.synthesize(truthTable, care);
  } else {
    synthesis::MMSynthesizer isop;
    resynthesizedId = isop.synthesizeWithFactoring(truthTable, care, maxArity);
  }

  auto &resynthesized = Subnet::get(resynthesizedId);
  auto resynthesizedTT = gate::model::evaluateSingleOut(resynthesized);
  if (care.num_vars() > 0) {
    resynthesizedTT &= care;
    truthTable &= care;
  }
  assert(resynthesizedTT == truthTable && "Resynthesized TT != Input TT\n");
  return resynthesizedId;
}

} // namespace eda::gate::optimizer
