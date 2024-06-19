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
    const SubnetSpace &space, const optimizer::CutExtractor::Cut &cut) {
  std::vector<optimizer::CostVector> vectors;
  vectors.reserve(cut.entryIdxs.size());

  for (const auto entryID : cut.entryIdxs) {
    vectors.push_back(getCostVector(space, entryID));
  }

  return vectors;
}

static optimizer::CostVector defaultCostAggregator(
   const std::vector<optimizer::CostVector> &vectors) {
  optimizer::CostVector result = optimizer::CostVector::Zero;

  for (size_t i = 0; i < vectors.size(); ++i) {
    const auto &vector = vectors[i];
    assert(vector.size() >= optimizer::CostVector::DefaultSize);

    result[optimizer::AREA] += vector[optimizer::AREA];
    result[optimizer::DELAY] = std::max(result[optimizer::DELAY],
                                        vector[optimizer::DELAY]);
    result[optimizer::POWER] += vector[optimizer::POWER];
  }

  return result;
}

static optimizer::CostVector defaultCostPropagator(
    const optimizer::CostVector &vector, const uint32_t fanout) {
  optimizer::CostVector result;

  result[optimizer::AREA]  = vector[optimizer::AREA] / fanout;
  result[optimizer::DELAY] = vector[optimizer::DELAY];
  result[optimizer::POWER] = vector[optimizer::POWER] / fanout;

  return result;
}

SubnetTechMapper::SubnetTechMapper(const std::string &name,
                                   const optimizer::Criterion &criterion,
                                   const CutProvider cutProvider,
                                   const MatchFinder matchFinder,
                                   const CellEstimator cellEstimator):
  SubnetTechMapper(name,
                   criterion,
                   cutProvider,
                   matchFinder,
                   cellEstimator,
                   defaultCostAggregator,
                   defaultCostPropagator) {}

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

  auto builder = std::make_shared<model::SubnetBuilder>();

  // Maps old entry indices to new links.
  std::vector<model::Subnet::Link> links(subnet.size());

  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &oldCell = entries[i].cell;

    if (!matches[i]) {
      continue;
    }

    const auto &type = model::CellType::get(matches[i]->typeID);
    assert(type.getInNum() == matches[i]->links.size());

    model::Subnet::LinkList newLinks(type.getInNum());
    for (size_t j = 0; j < type.getInNum(); ++j) {
      const auto oldLink = matches[i]->links[j];
      const auto newLink = links[oldLink.idx];
      newLinks[j] = oldLink.inv ? ~newLink : newLink;
      assert(!(type.isCell() && newLinks[j].inv)
          && "Inversions in a techmapped net are not allowed");
    }
    
    const auto link = builder->addCell(matches[i]->typeID, newLinks);
    links[i] = matches[i]->inversion ? ~link : link;

    if (oldCell.isIn() || oldCell.isOut()) {
      auto &newCell = builder->getCell(link.idx);
      newCell.flipFlop = oldCell.flipFlop;
      newCell.flipFlopID = oldCell.flipFlopID;
    }
  }

  return builder;
}

static inline float getProgress(const model::Subnet &subnet, const size_t i) {
  const auto nIn = subnet.getInNum();
  const auto nOut = subnet.getOutNum();
  const auto nAll = subnet.size();

  // Ignore the subnet inputs.
  if (i < nIn) { return 0.0; }

  // Last non-output cell index.
  const auto k = nAll - nOut - 1;
  const auto j = (i > k) ? k : i;

  // Progress reaches 100% when merging outputs.
  return static_cast<float>(j + 1 - nIn) /
         static_cast<float>(nAll - nIn);
}

optimizer::SubnetBuilderPtr SubnetTechMapper::make(
    const model::SubnetID subnetID) const {
  const auto &subnet = model::Subnet::get(subnetID);
  const auto &entries = subnet.getEntries();

  // Partial solutions for the subnet cells.
  SubnetSpace space(subnet.size());
  optimizer::CostVector tension{1.0, 1.0, 1.0};

  // Maximum number of tries for recovery.
  constexpr size_t maxTries{3};
  size_t tryCount = 0;

RECOVERY:
  tryCount++;

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto progress = getProgress(subnet, i);
    assert(0.0 <= progress && progress <= 1.0);

    space[i] = std::make_unique<CellSpace>(criterion, tension, progress);

    // Handle the input cells.
    if (i < subnet.getInNum()) {
      const Match match{model::getCellTypeID(model::IN), {}, false};
      space[i]->add(match, optimizer::CostVector::Zero);
      continue;
    }

    // Handle the output cells.
    if (i >= subnet.size() - subnet.getOutNum()) {
      const auto link = subnet.getLink(i, 0);
      const Match match{model::getCellTypeID(model::OUT), {link}, false};
      space[i]->add(match, space[link.idx]->getBest().vector);
      continue;
    }

    const auto &cell = entries[i].cell;
    const auto cuts = cutProvider(subnet, i);

    for (const auto &cut : cuts) {
      const auto cutCostVectors = getCostVectors(space, cut);
      const auto cutAggregation = costAggregator(cutCostVectors);

      if (!criterion.check(cutAggregation)) {
        continue;
      }

      const auto matches = matchFinder(subnet, cut);

      for (const auto &match : matches) {
        const auto cellCostVector = cellEstimator(match.typeID, Context{});
        const auto costVector = cutAggregation + cellCostVector;

        if (!criterion.check(costVector)) {
          continue;
        }

        space[i]->add(match, costPropagator(costVector, cell.refcount));
      }
    } // for cuts

    if (progress > 0.5 && !space[i]->hasFeasible()) {
      // Do recovery.
      if (tryCount < maxTries) {
        tension *= space[i]->getTension();
        goto RECOVERY;
      }
    }

    i += cell.more;
  } // for cells

  std::unordered_set<size_t> outputs;
  outputs.reserve(subnet.getOutNum());

  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    outputs.insert(subnet.size() - 1 - i);
  }  

  optimizer::CutExtractor::Cut resultCut(model::OBJ_NULL_ID, 0, outputs);
  const auto subnetCostVectors = getCostVectors(space, resultCut);
  const auto subnetAggregation = costAggregator(subnetCostVectors);

  if (!criterion.check(subnetAggregation)) {
    // Do recovery.
    if (tryCount < maxTries) {
      tension *= criterion.getTension(subnetAggregation);
      goto RECOVERY;
    }
  }

  return makeBuilder(space, subnet);  
}

} // namespace eda::gate::techmapper
