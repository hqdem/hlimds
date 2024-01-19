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
using LinkList = MinatoMorrealeAlg::LinkList;

Link MinatoMorrealeAlg::run(const KittyTT &func,
                            const LinkList &inputs,
                            SubnetBuilder &subnetBuilder,
                            uint16_t maxArity) const {
  return synthFromISOP(kitty::isop(func),
                       inputs,
                       subnetBuilder,
                       maxArity);
}

Link MinatoMorrealeAlg::synthFromISOP(const ISOP &cubes,
                                      const LinkList &inputs,
                                      SubnetBuilder &subnetBuilder,
                                      uint16_t maxArity) const {
  if (cubes.size() == 1) {
    return synthFromCube(cubes[0], inputs, subnetBuilder, maxArity);
  }

  LinkList links;
  for (auto it = cubes.begin(); it < cubes.end(); ++it) {
    Link link = synthFromCube(*it, inputs, subnetBuilder, maxArity);
    links.push_back(~link);
  }
  
  return ~subnetBuilder.addCellTree(model::AND, links, maxArity);
}

Link MinatoMorrealeAlg::synthFromCube(Cube cube,
                                      const LinkList &inputs,
                                      SubnetBuilder &subnetBuilder,
                                      uint16_t maxArity) const {
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

} // namespace eda::gate::optimizer2::resynthesis
