//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/lazy_refactorer.h"

#include <functional>

namespace eda::gate::optimizer {

using SubnetBuilder = eda::gate::model::SubnetBuilder;  
using Subnet = eda::gate::model::Subnet;

using Visitor = std::function<bool(SubnetBuilder &builder,
                                   const bool isIn,
                                   const bool isOut,
                                   const size_t entryID)>;

void LazyRefactorer::transform(
    const std::shared_ptr<SubnetBuilder> &builderPtr) const {
  SubnetBuilder *builder = builderPtr.get();
  ConflictGraph g;
  if (weightCalculator) {
    (*weightCalculator)(*builder, {});
  }
  for (auto iter = builder->begin(); 
       iter != builder->end() && !builder->getCell(*iter).isOut();
       ++iter) {
    nodeProcessing(*builder, iter, g);
  }
  g.findBestColoring(builder);
}

void LazyRefactorer::nodeProcessing(SubnetBuilder &builder,
                                    EntryIterator &iter, 
                                    ConflictGraph &g) const {

  const size_t entryID{*iter};
  SubnetView view = (*coneConstructor)(builder, entryID);
  const size_t coneIns{view.getInNum()};

  auto newCone = resynthesizer.resynthesize(view, 2);
  if (newCone.isNull()) {
    return;
  }
  SubnetBuilder &newConeBuilder = newCone.builder();

  if (weightCalculator) {
    std::vector<float> weights;
    weights.reserve(coneIns);
    for (size_t i = 0; i < coneIns; ++i) {
      weights.push_back(builder.getWeight(view.getIn(i)));
    }
    (*weightCalculator)(newConeBuilder, weights);
  }

  auto newConeMap = view.getInOutMapping();
  auto effect = builder.evaluateReplace(newCone, newConeMap, weightModifier);

  SubnetView newWindow(builder, newConeMap);
  
  if ((effect.weight - eps) > eps) {

    std::vector<size_t> numCells;
    bool open = false;
    SubnetViewWalker walker(newWindow);
    auto add2Graph = [&numCells, &open] (SubnetBuilder &builder,
                                         const bool isIn,
                                         const bool isOut,
                                         const size_t entryID) -> bool {
      
      if ((!isIn && !isOut) && (builder.getCell(entryID).refcount > 1)) {
        open = true;
        return false;
      }
      numCells.push_back(entryID);
      return true;
    };

    walker.run(add2Graph);
    if (!open) {
      g.addVertice(effect.weight, newCone.make(), newConeMap, numCells);
    }
  }
}

eda::gate::model::SubnetView LazyRefactorer::twoLvlBldr(SubnetBuilder &builder, 
                                                        size_t numCell) {

  eda::gate::model::InOutMapping entryMap;
  const auto curLinks = builder.getLinks(numCell);

  if (curLinks.empty()) {
    entryMap.inputs.push_back(numCell);
  }
  
  for (auto curLink : curLinks) {

    if (builder.getLinks(curLink.idx).empty()) {     
      entryMap.inputs.push_back(curLink.idx);
    } else {

      const Subnet::LinkList prevLinks = builder.getLinks(curLink.idx);
     
      for (auto l : prevLinks) {
        entryMap.inputs.push_back(l.idx);
      }
    }
  }

  entryMap.outputs.push_back(numCell);
  return SubnetView(builder, entryMap);
}

} // namespace eda::gate::optimizer
