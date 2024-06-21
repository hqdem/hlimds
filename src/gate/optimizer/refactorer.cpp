//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "reconvergence_cut.h"
#include "refactorer.h"

#include "gate/analyzer/probabilistic_estimate.h"

namespace eda::gate::optimizer {

void Refactorer::transform(SubnetBuilder &builder) const {
  if (weightCalculator) {
    (*weightCalculator)(builder, {});
  }
  for (SafePasser iter(builder.begin()); 
       iter != builder.end() && !builder.getCell(*iter).isOut();++iter) {
    nodeProcessing(builder, iter);
  }
}

void Refactorer::nodeProcessing(SubnetBuilder &builder, SafePasser &iter) const {
  const size_t entryID{*iter};
  EntryMap oldConeMap;
  SubnetID oldConeID
      = (*coneConstructor)(builder, entryID, cutSize, oldConeMap);

  const size_t coneIns{Subnet::get(oldConeID).getInNum()};

  ResynthesizerBase::TruthTable care;
  (void)careCutSize; // FIXME:
  /* FIXME: Uncomment.
  if (careCutSize > cutSize) {
    std::vector<size_t> roots;
    roots.reserve(coneIns);
    for (size_t i = 0; i < coneIns; ++i) {
      roots.push_back(oldConeMap.at(i));
    }
    std::unordered_map<size_t, size_t> dummy;
    SubnetID careSubnetId
        = getReconvergenceWindow(builder, roots, careCutSize, dummy);
    const auto &careSubnet = Subnet::get(careSubnetId);
    care = gate::model::computeCare(careSubnet);
  }
  */

  //--- FIXME:
  CutExtractor::Cut cut;
  cut.rootEntryIdx = oldConeMap.at(Subnet::get(oldConeID).size() - 1);
  for (size_t i = 0; i < coneIns; ++i) {
    cut.entryIdxs.insert(oldConeMap.at(i));
  }
  //---

  std::unordered_map<size_t, size_t> mapping; // FIXME:
  SubnetID newConeID = resynthesizer.resynthesize(builder, cut, mapping, care, 2); // FIXME: oldConeID, care, 2);
  EntryMap newConeMap;

  for (size_t i = 0; i < coneIns; ++i) {
    newConeMap[i] = mapping[i]; // FIXME: oldConeMap[i];
  }
  newConeMap[Subnet::get(newConeID).size() - 1] = cut.rootEntryIdx;
      // FIXME: oldConeMap.at(Subnet::get(oldConeID).size() - 1);

  SubnetBuilder newConeBuilder(newConeID);

  if (weightCalculator) {
    std::vector<float> weights;
    weights.reserve(coneIns);
    for (size_t i = 0; i < coneIns; ++i) {
      weights.push_back(builder.getWeight(mapping.at(i)/* FIXME: oldConeMap.at(i)*/));
    }
    (*weightCalculator)(newConeBuilder, weights);
  }

  auto effect
      = builder.evaluateReplace(newConeBuilder, newConeMap, weightModifier);
  if ((*replacePredicate)(effect)) {
    iter.replace(newConeBuilder, newConeMap);
  }

}

} // namespace eda::gate::optimizer
