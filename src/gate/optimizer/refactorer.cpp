//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/reconvergence_cut.h"
#include "gate/optimizer/refactorer.h"

namespace eda::gate::optimizer {

void Refactorer::transform(const SubnetBuilderPtr &builder) const {
  SubnetBuilder *builderPtr = builder.get();

  if (weightCalculator) {
    (*weightCalculator)(*builderPtr, {});
  }
  for (SafePasser iter(builderPtr->begin()); 
       iter != builderPtr->end() && !builderPtr->getCell(*iter).isOut();
       ++iter) {
    nodeProcessing(*builderPtr, iter);
  }
}

void Refactorer::nodeProcessing(SubnetBuilder &builder, SafePasser &iter) const {
  const size_t entryID{*iter};
  EntryMap oldConeMap;
  SubnetID oldConeID
      = (*coneConstructor)(builder, entryID, cutSize, oldConeMap);

  const size_t coneIns{Subnet::get(oldConeID).getInNum()};

  //--- FIXME: Window should be constructed by window constructor.
  std::vector<size_t> inputs(coneIns);
  for (size_t i = 0; i < inputs.size(); ++i) {
    inputs[i] = oldConeMap.at(i);
  }

  std::vector<size_t> outputs(1);
  outputs[0] = oldConeMap.at(Subnet::get(oldConeID).size() - 1);

  SubnetView window(builder, InOutMapping{inputs, outputs});
  //---

  if (careCutSize > cutSize) {
    const auto &roots = window.getInputs();
    std::unordered_map<size_t, size_t> dummy;
    const SubnetID careSubnetId
        = getReconvergenceWindow(builder, roots, careCutSize, dummy);
    const auto &careSubnet = Subnet::get(careSubnetId);
    window.setCare(model::computeCare(careSubnet));
  }

  auto newCone = resynthesizer.resynthesize(window, 2);
  SubnetBuilder &newConeBuilder = newCone.builder();

  if (weightCalculator) {
    std::vector<float> weights;
    weights.reserve(coneIns);
    for (size_t i = 0; i < coneIns; ++i) {
      weights.push_back(builder.getWeight(window.getIn(i)));
    }
    (*weightCalculator)(newConeBuilder, weights);
  }

  auto newConeMap = window.getInOutMapping();

  auto effect
      = builder.evaluateReplace(newCone, newConeMap, weightModifier);
  if ((*replacePredicate)(effect)) {
    iter.replace(newCone, newConeMap);
  }
}

} // namespace eda::gate::optimizer
