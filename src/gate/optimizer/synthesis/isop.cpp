//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/isop.h"

namespace eda::gate::optimizer::synthesis {

using TruthTable = MMSynthesizer::TruthTable;

Link synthFromSOP(const SOP &sop, const LinkList &inputs,
                  SubnetBuilder &subnetBuilder, uint16_t maxArity) {
  if (sop.size() == 1) {
    return synthFromCube(sop[0], inputs, subnetBuilder, maxArity);
  }

  LinkList links;
  for (auto it = sop.begin(); it < sop.end(); ++it) {
    Link link = synthFromCube(*it, inputs, subnetBuilder, maxArity);
    links.push_back(~link);
  }

  return ~subnetBuilder.addCellTree(model::AND, links, maxArity);
}

Link synthFromCube(Cube cube, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, int16_t maxArity) {
  uint32_t mask {cube._mask};
  LinkList links;
  for (; mask; mask &= (mask - 1)) {
    size_t idx = std::log2(mask - (mask & (mask - 1)));
    bool inv = !(cube.get_bit(idx));
    links.push_back(Link(inputs[idx].idx, inv));
  }
  if (links.size() == 1) {
    return links[0];
  }
  return subnetBuilder.addCellTree(model::AND, links, maxArity);
}

model::SubnetID MMSynthesizer::synthesize(const TruthTable &func,
                                          const TruthTable &care,
                                          uint16_t maxArity) const {                              
  SubnetBuilder subnetBuilder;
  LinkList ins = subnetBuilder.addInputs(func.num_vars());

  const auto [tt, inv] = handleCare(func, care);
  
  if (auto id{checkConstTT(tt, inv)}; id != model::OBJ_NULL_ID) {
    return id;
  }

  Link output{synthFromSOP(kitty::isop(tt), ins, subnetBuilder, maxArity)};                               
  subnetBuilder.addOutput(inv ? ~output : output);
  return subnetBuilder.make();
}

model::SubnetID MMSynthesizer::synthesizeWithFactoring(const TruthTable &func,
                                                       const TruthTable &care,
                                                       uint16_t maxArity)
                                                       const {

  const auto [tt, inv] = handleCare(func, care);

  if (auto id{checkConstTT(tt, inv)}; id != model::OBJ_NULL_ID) {
    return id;
  }

  auto numVars = tt.num_vars();
  return factor.getSubnet(kitty::isop(tt), numVars, maxArity, inv);
}

std::pair<TruthTable, bool> MMSynthesizer::handleCare(
    const TruthTable &func, const TruthTable &care) const {
  if (care.num_vars() == 0) {
    bool inv{kitty::count_ones(func) > (func.num_bits() / 2)};
    return {inv ? ~func : func, inv};
  }
  auto funcWithCare{func & care};
  auto inverseFuncWithCare{(~func) & care};

  bool inv{kitty::count_ones(funcWithCare) > 
      kitty::count_ones(inverseFuncWithCare)};
    
  return {inv ? inverseFuncWithCare : funcWithCare, inv};
}

} // namespace eda::gate::optimizer::synthesis
