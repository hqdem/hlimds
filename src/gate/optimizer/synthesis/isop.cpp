//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/isop.h"

namespace eda::gate::optimizer::synthesis {

model::SubnetID MMSynthesizer::synthesize(const KittyTT &func,
                                          const KittyTT &care,
                                          uint16_t maxArity) const {

  SubnetBuilder subnetBuilder;
  LinkList ins = subnetBuilder.addInputs(func.num_vars());

  auto funcWithCare = func & care;
  auto notFuncWithCare = (~func) & care;

  const size_t nFuncOnes = kitty::count_ones(funcWithCare);
  const size_t nNotFuncOnes = kitty::count_ones(notFuncWithCare);

  bool inv = nFuncOnes > nNotFuncOnes;
  funcWithCare = inv ? notFuncWithCare : funcWithCare;

  CONST_CHECK(funcWithCare, inv)

  const auto &isop = kitty::isop(funcWithCare);

  Link output = inv ?
      ~utils::synthFromSOP(isop, ins, subnetBuilder, maxArity) :
      utils::synthFromSOP(isop, ins, subnetBuilder, maxArity);                               
  subnetBuilder.addOutput(output);
  return subnetBuilder.make();
}

model::SubnetID MMSynthesizer::synthesizeWithFactoring(const KittyTT &func,
                                                       const KittyTT &care,
                                                       uint16_t maxArity)
                                                       const {

  auto funcWithCare = func & care;
  auto notFuncWithCare = (~func) & care;

  const size_t nFuncOnes = kitty::count_ones(funcWithCare);
  const size_t nNotFuncOnes = kitty::count_ones(notFuncWithCare);

  bool inv = nFuncOnes > nNotFuncOnes;
  funcWithCare = inv ? notFuncWithCare : funcWithCare;

  CONST_CHECK(funcWithCare, inv)

  auto numVars = func.num_vars();
  return factor.getSubnet(kitty::isop(funcWithCare), numVars, maxArity, inv);
}

} // namespace eda::gate::optimizer::synthesis
