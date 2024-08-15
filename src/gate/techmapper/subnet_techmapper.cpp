//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/techmapper/subnet_techmapper.h"

#include <cassert>
#include <memory>
#include <queue>
#include <unordered_set>

// Disables cut extraction and matching for outputs.
#define UTOPIA_TECHMAP_NO_MATCHING_FOR_OUTPUTS 0

namespace eda::gate::techmapper {

using CellSpace = criterion::SolutionSpace<SubnetTechMapper::Match>;
using SubnetSpace = std::vector<std::unique_ptr<CellSpace>>;

static inline const criterion::CostVector &getCostVector(
    const SubnetSpace &space, const size_t entryID) {
  return space[entryID]->getBest().vector;
}

static inline std::vector<criterion::CostVector> getCostVectors(
    const SubnetSpace &space, const optimizer::CutExtractor::Cut &cut) {
  std::vector<criterion::CostVector> vectors;
  vectors.reserve(cut.entryIdxs.size());

  for (const auto entryID : cut.entryIdxs) {
    vectors.push_back(getCostVector(space, entryID));
  }

  return vectors;
}

static criterion::CostVector defaultCostAggregator(
   const std::vector<criterion::CostVector> &vectors) {
  criterion::CostVector result = criterion::CostVector::Zero;

  for (size_t i = 0; i < vectors.size(); ++i) {
    const auto &vector = vectors[i];
    assert(vector.size() >= criterion::CostVector::DefaultSize);

    result[criterion::AREA] += vector[criterion::AREA];
    result[criterion::DELAY] = std::max(result[criterion::DELAY],
                                        vector[criterion::DELAY]);
    result[criterion::POWER] += vector[criterion::POWER];
  }

  return result;
}

static criterion::CostVector defaultCostPropagator(
    const criterion::CostVector &vector, const uint32_t fanout) {
  criterion::CostVector result;

  result[criterion::AREA]  = vector[criterion::AREA] / fanout;
  result[criterion::DELAY] = vector[criterion::DELAY];
  result[criterion::POWER] = vector[criterion::POWER] / fanout;

  return result;
}

SubnetTechMapper::SubnetTechMapper(const std::string &name,
                                   const criterion::Criterion &criterion,
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

/// Maps a entry ID to the best match (null stands for no match).
using MatchSelection = std::vector<const SubnetTechMapper::Match*>;

static void selectBestMatches(const SubnetSpace &space,
                              const optimizer::SubnetBuilderPtr oldBuilder,
                              MatchSelection &matches) {
  // Traverse the subnet in reverse order starting from the outputs.
  std::queue<size_t> queue;
  for (auto it = --oldBuilder->end(); it != oldBuilder->begin(); --it) {
    const auto entryID = *it;
    const auto &oldCell = oldBuilder->getCell(entryID);

    if (!oldCell.isOut()) break;
    queue.push(entryID);
  } // for output

  while (!queue.empty()) {
    const auto entryID = queue.front();
    queue.pop();

    matches[entryID] = &space[entryID]->getBest().solution;

    for (const auto &oldLink : matches[entryID]->links) {
      if (!matches[oldLink.idx]) {
        queue.push(oldLink.idx);
      }
    }
  } // backward BFS
}

static optimizer::SubnetBuilderPtr makeMappedSubnet(
    const SubnetSpace &space, const optimizer::SubnetBuilderPtr oldBuilder) {
  // Maps old entry indices to matches.
  const size_t oldSize = oldBuilder->getMaxIdx() + 1;
  MatchSelection matches(oldSize);

  // Find best coverage.
  selectBestMatches(space, oldBuilder, matches);

  auto newBuilder = std::make_shared<model::SubnetBuilder>();

  // Maps old entry indices to new links.
  std::vector<model::Subnet::Link> links(oldSize);

  for (auto it = oldBuilder->begin(); it != oldBuilder->end(); ++it) {
    const auto entryID = *it;
    const auto &oldCell = oldBuilder->getCell(entryID);

    if (oldCell.isIn()) {
      // Add all inputs even if some of them are not used (matches[i] is null).
      links[entryID] = newBuilder->addInput();
    } else if (matches[entryID]) {
      const auto &type = model::CellType::get(matches[entryID]->typeID);
      assert(type.getInNum() == matches[entryID]->links.size());

      model::Subnet::LinkList newLinks(type.getInNum());
      for (size_t j = 0; j < type.getInNum(); ++j) {
        const auto oldLink = matches[entryID]->links[j];
        const auto newLink = links[oldLink.idx];
        newLinks[j] = oldLink.inv ? ~newLink : newLink;
        assert(!(type.isCell() && newLinks[j].inv)
            && "Inversions in a techmapped net are not allowed");
      }
      
      const auto link = newBuilder->addCell(matches[entryID]->typeID, newLinks);
      links[entryID] = matches[entryID]->inversion ? ~link : link;

      const auto isOldOut = oldCell.isOut();
      const auto isNewOut = type.isOut();

#if UTOPIA_TECHMAP_NO_MATCHING_FOR_OUTPUTS
      assert(isOldOut == isNewOut);
#else
      if (isOldOut && !isNewOut) {
        newBuilder->addOutput(link);
      }
#endif // UTOPIA_TECHMAP_NO_MATCHING_FOR_OUTPUTS
    }

    if (oldCell.isIn() || oldCell.isOut()) {
      auto &newCell = newBuilder->getCell(links[entryID].idx);
      newCell.flipFlop = oldCell.flipFlop;
      newCell.flipFlopID = oldCell.flipFlopID;
    }
  } // for cell

  const auto oldOutNum = oldBuilder->getOutNum();
  const auto newOutNum = newBuilder->getOutNum();
  if (newOutNum != oldOutNum) {
    UTOPIA_ERROR("Incorrect number of outputs in the tech-mapped subnet: "
        << newOutNum << ", expected " << oldOutNum);
    return nullptr /* error */;
  }

  return newBuilder;
}

static inline float getProgress(const optimizer::SubnetBuilderPtr &builder,
                                const size_t count) {
  const auto nIn = builder->getInNum();
  const auto nOut = builder->getOutNum();
  const auto nAll = builder->getCellNum();

  // Ignore the subnet inputs.
  if (count < nIn) { return 0.0; }

  // Last non-output cell index.
  const auto k = nAll - nOut - 1;
  const auto j = (count > k) ? k : count;

  // Progress reaches 100% when merging outputs.
  return static_cast<float>(j + 1 - nIn) /
         static_cast<float>(nAll - nIn);
}

optimizer::SubnetBuilderPtr SubnetTechMapper::map(
    const optimizer::SubnetBuilderPtr &builder) const {
  // Partial solutions for the subnet cells.
  SubnetSpace space(builder->getMaxIdx() + 1);
  criterion::CostVector tension{1.0, 1.0, 1.0};

  // Subnet outputs to be filled in the loop below.
  std::unordered_set<size_t> outputs;
  outputs.reserve(builder->getOutNum());

  // Maximum number of tries for recovery.
  constexpr size_t maxTries{3};
  size_t tryCount = 0;

RECOVERY:
  tryCount++;

  size_t cellCount{0};
  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto progress = getProgress(builder, cellCount++);
    assert(0.0 <= progress && progress <= 1.0);

    const auto entryID = *it;
    const auto &cell = builder->getCell(entryID);

    space[entryID] = std::make_unique<CellSpace>(criterion, tension, progress);

    // Handle the input cells.
    if (cell.isIn()) {
      const Match match{model::getCellTypeID(model::IN), {}, false};
      space[entryID]->add(match, criterion::CostVector::Zero);
      continue;
    }

    // Handle the output cells.
    if (cell.isOut()) {
      outputs.insert(entryID);

#if UTOPIA_TECHMAP_NO_MATCHING_FOR_OUTPUTS
      const auto link = builder->getLink(entryID, 0);
      const Match match{model::getCellTypeID(model::OUT), {link}, false};
      space[entryID]->add(match, space[link.idx]->getBest().vector);
      continue;
#endif // UTOPIA_TECHMAP_NO_MATCHING_FOR_OUTPUTS
    }

    const auto cuts = cutProvider(*builder, entryID);

    for (const auto &cut : cuts) {
      assert(cut.rootEntryIdx == entryID);

      // Skip trivial cuts.
      if (cut.isTrivial()) {
        continue;
      }

      const auto cutCostVectors = getCostVectors(space, cut);
      const auto cutAggregation = costAggregator(cutCostVectors);

      if (!criterion.check(cutAggregation)) {
        continue;
      }

      const auto matches = matchFinder(*builder, cut);

      for (const auto &match : matches) {
        const auto cellCostVector = cellEstimator(match.typeID, Context{});
        const auto costVector = cutAggregation + cellCostVector;

        if (!criterion.check(costVector)) {
          continue;
        }

        space[entryID]->add(match, costPropagator(costVector, cell.refcount));
      } // for match
    } // for cuts

    if (!space[entryID]->hasSolution()) {
      UTOPIA_ERROR("No match found for "
          << fmt::format("cell#{}:{}", entryID, cell.getType().getName()));
      return nullptr /* error */;
    }

    if (progress > 0.5 && !space[entryID]->hasFeasible()) {
      // Do recovery.
      if (tryCount < maxTries) {
        tension *= space[entryID]->getTension();
        goto RECOVERY;
      }
    }
  } // for cells

  assert(outputs.size() == builder->getOutNum());
  optimizer::CutExtractor::Cut resultCut(outputs.size(), model::OBJ_NULL_ID, 0, outputs);
  const auto subnetCostVectors = getCostVectors(space, resultCut);
  const auto subnetAggregation = costAggregator(subnetCostVectors);

  if (!criterion.check(subnetAggregation)) {
    // Do recovery.
    if (tryCount < maxTries) {
      tension *= criterion.getTension(subnetAggregation);
      goto RECOVERY;
    }
  }

  return makeMappedSubnet(space, builder);  
}

} // namespace eda::gate::techmapper
