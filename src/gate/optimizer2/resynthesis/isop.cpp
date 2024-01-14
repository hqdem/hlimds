//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/isop.h"

namespace eda::gate::optimizer2::resynthesis {

using Link = MinatoMorrealeAlg::Link;

Link MinatoMorrealeAlg::run(const KittyTT &func,
                            const Inputs &inputs,
                            SubnetBuilder &subnetBuilder,
                            uint16_t maxArity) const {
  return synthFromISOP(kitty::isop(func),
                       inputs,
                       subnetBuilder,
                       maxArity);
}

Link MinatoMorrealeAlg::synthFromISOP(const ISOP &cubes,
                                      const Inputs &inputs,
                                      SubnetBuilder &subnetBuilder,
                                      uint16_t maxArity) const {
  if (cubes.size() == 1) {
    return synthFromCube(cubes[0], inputs, subnetBuilder, maxArity);
  }

  LinkList links;
  for (auto it = cubes.begin(); it < cubes.end(); ++it) {
    Link link = synthFromCube(*it, inputs, subnetBuilder, maxArity);
    link.inv = ~link.inv;
    links.push_back(std::move(link));
  }
  return Link(static_cast<uint32_t>(
    subnetBuilder.addCellTree(model::AND, links, maxArity)), true);
}

Link MinatoMorrealeAlg::synthFromCube(Cube cube,
                                      const Inputs &inputs,
                                      SubnetBuilder &subnetBuilder,
                                      uint16_t maxArity) const {
  uint32_t mask {cube._mask};
  LinkList links;
  for (; mask; mask &= (mask - 1)) {
    size_t idx = std::log2(mask - (mask & (mask - 1)));
    bool inv = !(cube.get_bit(idx));
    links.push_back(Link(inputs[idx], inv));
  }
  if (links.size() == 1) {
    return links[0];
  }
  return Link(static_cast<uint32_t>(
      subnetBuilder.addCellTree(model::AND, links, maxArity)));
}

} // namespace eda::gate::optimizer2::resynthesis
