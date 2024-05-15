//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer/depth_replacer.h"

namespace eda::gate::optimizer {

void DepthReplacer::replace(SubnetFragment lhs, SubnetID rhs) {
  const auto &initialSubnet = model::Subnet::get(lhs.subnetID);
  const auto &subnet = model::Subnet::get(rhs);
  std::unordered_map<size_t, size_t> map;
  assert(subnet.getInNum() == initialSubnet.getInNum()
         && "Incorrect number of PIs after resynthesis");
  for (int i = 0; i < subnet.getInNum(); ++i) {
    map.insert(std::make_pair(i, lhs.entryMap[i]));
  }

  /*
   * It is necessary that "out" of the optimised cone
   * points to the root in the big scheme.
   * out - in the optimised cone is the last gate.
   */
  size_t outIndex = subnet.getEntries().size() - 1;
  // there's no out in the cone
  size_t rootIndex = lhs.entryMap.size() - 1;
  size_t originaRootIndex = lhs.entryMap[rootIndex];
  map.insert(std::make_pair(outIndex, originaRootIndex));
  if (subnetBuilder.evaluateReplace(rhs, map).depth > 0) {
    assert(evaluate(initialSubnet) == evaluate(subnet)
           && "Subnet equality failed!");
    subnetBuilder.replace(rhs, map);
  }
}

} // namespace eda::gate::optimizer
