//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/subnet_techmapper.h"

#include <cassert>
#include <memory>
#include <queue>
#include <unordered_set>

namespace eda::gate::techmapper {

using CellSpace = optimizer::SolutionSpace<SubnetTechMapper::Match>;
using SubnetSpace = std::vector<std::unique_ptr<CellSpace>>;

static inline const optimizer::CostVector &getCostVector(
    const SubnetSpace &space, const size_t entryID) {
  return space[entryID]->getBest().vector;
}

static inline std::vector<optimizer::CostVector> getCostVectors(
    const SubnetSpace &space, const SubnetTechMapper::Cut &cut) {
  std::vector<optimizer::CostVector> vectors;
  vectors.reserve(cut.entryIdxs.size());
  for (const auto entryID : cut.entryIdxs) {
    vectors.push_back(getCostVector(space, entryID));
  }
  return vectors;
}

static optimizer::SubnetBuilderPtr makeBuilder(
    const SubnetSpace &space, const model::Subnet &subnet) {
  // Maps old entry indices to matches.
  std::vector<const SubnetTechMapper::Match*> matches(subnet.size());

  // Traverse the subnet in reverse order starting from the outputs.
  std::queue<size_t> queue;
  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    queue.push(subnet.size() - subnet.getOutNum() + i);
  }

  while (!queue.empty()) {
    const auto entryID = queue.front();
    queue.pop();

    matches[entryID] = &space[entryID]->getBest().solution;
    assert(matches[entryID]);

    for (const auto &link : matches[entryID]->links) {
      if (!matches[link.idx]) {
        queue.push(link.idx);
      }
    }
  }

  const auto builder = std::make_shared<model::SubnetBuilder>();

  // Maps old entry indices to new links.
  std::vector<model::Subnet::Link> links(subnet.size());

  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = entries[i].cell;

    if (!matches[i]) {
      continue;
    }

    model::Subnet::LinkList newLinks(cell.arity);
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto oldLink = subnet.getLink(i, j);
      const auto newLink = links[oldLink.idx];
      newLinks[j] = oldLink.inv ? ~newLink : newLink; // FIXME: handle inversion.
    }

    
    const auto link = builder->addCell(matches[i]->typeID, newLinks);
    links[i] = matches[i]->inversion ? ~link : link; // FIXME: handle inversion.

    // FIXME: mark flip-flop inputs and outputs.
  }

  return builder;
}

optimizer::SubnetBuilderPtr SubnetTechMapper::make(
    const model::SubnetID subnetID) const {
  const auto &subnet = model::Subnet::get(subnetID);
  const auto &entries = subnet.getEntries();

  // Partial solutions for subnet cells.
  SubnetSpace space(subnet.size());

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

      const auto matches = cellTypeProvider(subnet, cut); // FIXME: Rename

      for (const auto &match : matches) {
        const auto cellCostVector = cellTypeEstimator(match.typeID);
        const auto costVector = exactCostAggregator({cutCostVector, cellCostVector}); // TODO: always +?

        if (!criterion.check(costVector)) {
          continue; // TODO: soften checks?
        }

        space[i]->add(match, costVector);
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

  return makeBuilder(space, subnet);  
}

} // namespace eda::gate::techmapper
