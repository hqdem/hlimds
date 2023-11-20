//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/isop.h"

namespace eda::gate::optimizer2::resynthesis {

MinatoMorrealeAlg::Link MinatoMorrealeAlg::run(const KittyTT &func, 
    const Inputs &inputs, uint32_t &dummy, SubnetBuilder &subnetBuilder) const {
  return synthFromISOP(kitty::isop(func), inputs, dummy, subnetBuilder);    
}

MinatoMorrealeAlg::Link MinatoMorrealeAlg::synthFromISOP(const ISOP &cubes,
    const Inputs &inputs, uint32_t &dummy, SubnetBuilder &subnetBuilder) const {

  bool inv = (cubes.size() > 1);
  Link prev = synthFromCube(cubes[0], inputs, dummy, subnetBuilder); 
  if (!inv) {
    return prev;
  }
  prev.inv = ~prev.inv;
  for (auto it = cubes.begin() + 1; it < cubes.end() - 1; ++it) {
    Link tmp = synthFromCube(*it, inputs, dummy, subnetBuilder);
    tmp.inv = ~tmp.inv;
    prev = Link(subnetBuilder.addCell(model::AND, prev, tmp));
  }
  Link tmp = synthFromCube(*(cubes.end() - 1), inputs, dummy, subnetBuilder);
  tmp.inv = ~tmp.inv;

  return Link(subnetBuilder.addCell(model::AND, prev, tmp), true);
}

MinatoMorrealeAlg::Link MinatoMorrealeAlg::synthFromCube(Cube cube,
    const Inputs &inputs, uint32_t &dummy, SubnetBuilder &subnetBuilder) const {
  
  uint32_t mask { cube._mask };
  Link prev;
  bool first { true };
  for (; mask; mask &= (mask - 1)) {
    size_t idx = std::log2(mask - (mask & (mask - 1)));
    bool inv = !(cube.get_bit(idx));
    prev = (first) ?
        Link(inputs[idx], inv) : 
        Link(subnetBuilder.addCell(model::AND, prev, Link(inputs[idx], inv)));
    first = false;
    dummy &= ~(1u << inputs[idx]);
  }

  return prev;
}

} // namespace eda::gate::optimizer2::resynthesis
