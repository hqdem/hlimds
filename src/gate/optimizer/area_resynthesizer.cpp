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

#include <kitty/kitty.hpp>

namespace eda::gate::optimizer {

model::SubnetID AreaResynthesizer::resynthesize(
    const model::SubnetID subnetID,
    const TruthTable &care,
    const uint16_t maxArity) const {

  // Heuristic constants for optimizers calling.
  const size_t akersMaxInputNum = 8;
  const size_t akersArityNum    = 3;

  const auto &subnet = model::Subnet::get(subnetID);
  auto truthTable = gate::model::evaluateSingleOut(subnet);
  const auto numVars = truthTable.num_vars();

  model::SubnetID resynthesizedID = model::OBJ_NULL_ID;
  if ((numVars < akersMaxInputNum) && (maxArity == akersArityNum)) {
    synthesis::AkersSynthesizer akers;
    resynthesizedID = akers.synthesize(truthTable, care);
  } else {
    synthesis::MMFactorSynthesizer isop;
    resynthesizedID = isop.synthesize(truthTable, care, maxArity);
  }

#ifdef UTOPIA_DEBUG
  auto &resynthesized = model::Subnet::get(resynthesizedID);
  auto resynthesizedTT = model::evaluateSingleOut(resynthesized);
  if (care.num_vars() > 0) {
    resynthesizedTT &= care;
    truthTable &= care;
  }
  assert(resynthesizedTT == truthTable && "Resynthesized TT != Input TT\n");
#endif // UTOPIA_DEBUG

  return resynthesizedID;
}

} // namespace eda::gate::optimizer
