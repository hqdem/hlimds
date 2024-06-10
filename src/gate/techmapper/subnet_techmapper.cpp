//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/subnet_techmapper.h"

#include <memory>
#include <unordered_set>

namespace eda::gate::techmapper {

using CellSolution = std::pair<SubnetTechMapper::Cut, SubnetTechMapper::CellTypeID>;
using CellSpace = optimizer::SolutionSpace<CellSolution>;
using Space = std::vector<std::unique_ptr<CellSpace>>;

static inline const SubnetTechMapper::CostVector &getCostVector(
    const Space &space, const size_t entryID) {
  return space[entryID]->getBest().vector;
}

static inline const SubnetTechMapper::CostVectors getCostVectors(
    const Space &space, const SubnetTechMapper::Cut &cut) {
  SubnetTechMapper::CostVectors vectors;
  vectors.reserve(cut.entryIdxs.size());
  for (const auto entryID : cut.entryIdxs) {
    vectors.push_back(getCostVector(space, entryID));
  }
  return vectors;
}

SubnetTechMapper::SubnetBuilderPtr SubnetTechMapper::make(
    const SubnetID subnetID) const {
  const auto &subnet = Subnet::get(subnetID);
  const auto &entries = subnet.getEntries();

  // Partial solutions for subnet cells.
  Space space(subnet.size());

  for (size_t i = 0; i < entries.size(); ++i) {
    space[i] = std::make_unique<CellSpace>(criterion);

    // Ignore the input cells.
    if (i < subnet.getInNum()) {
      continue; // TODO:
    }

    const auto &cell = entries[i].cell;
    const auto cuts = cutProvider(subnet, i);

    for (const auto &cut : cuts) {
      const auto cutCostVector = flowCostAggregator(getCostVectors(space, cut));

      if (!criterion.check(cutCostVector)) {
        continue; // TODO: soften checks?
      }

      const auto cellTypeIDs = cellTypeProvider(subnet, cut);

      for (const auto &cellTypeID : cellTypeIDs) {
        const auto cellCostVector = cellTypeEstimator(cellTypeID);
        const auto costVector = exactCostAggregator({cutCostVector, cellCostVector}); // TODO: always +?

        if (!criterion.check(costVector)) {
          continue; // TODO: soften checks?
        }

        space[i]->add({cut, cellTypeID}, costVector);
      }
    } // for cuts

    if (!space[i]->hasFeasible()) {
      break; // TODO: recovery
    }

    i += cell.more;
  } // for cells

  std::unordered_set<size_t> outputs;
  outputs.reserve(subnet.getOutNum());

  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    outputs.insert(subnet.size() - i);
  }  

  Cut resultCut(model::OBJ_NULL_ID, 0, outputs);
  const auto subnetCostVector = flowCostAggregator(getCostVectors(space, resultCut)); // TODO: exact estimation at the end.

  if (!criterion.check(subnetCostVector)) {
    return nullptr; // TODO: recovery
  }

  // TODO: Compose the builder.
  return nullptr;  
}

} // namespace eda::gate::techmapper
