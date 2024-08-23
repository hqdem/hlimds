//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/model/subnetview.h"
#include "gate/techmapper/subnet_techmapper.h"

#include <cassert>
#include <memory>
#include <queue>
#include <unordered_set>

/// Disables cut extraction and matching for outputs.
#define UTOPIA_TECHMAP_MATCH_OUTPUTS 1

/// Saves mapped subnet points when selecting best matches.
#define UTOPIA_TECHMAP_SAVE_MAPPED_POINTS 1

/// Outputs the cost vector.
#define UTOPIA_LOG_COST_VECTOR(prefix, vector)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]  << std::endl\
      << "Delay: " << vector[criterion::DELAY] << std::endl\
      << "Power: " << vector[criterion::POWER] << std::endl)
 
namespace eda::gate::techmapper {

using CellSpace = criterion::SolutionSpace<SubnetTechMapper::Match>;
using SubnetSpace = std::vector<std::unique_ptr<CellSpace>>;

static inline bool hasSolutions(
    const SubnetSpace &space, const optimizer::CutExtractor::Cut &cut) {
  for (const auto entryID : cut.entryIdxs) {
    if (!space[entryID]->hasSolution()) {
      return false;
    }
  }
  return true;
}

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

  const auto divisor = fanout ? fanout : 1;
  result[criterion::AREA]  = vector[criterion::AREA] / divisor;
  result[criterion::DELAY] = vector[criterion::DELAY];
  result[criterion::POWER] = vector[criterion::POWER] / divisor;

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

static std::shared_ptr<model::SubnetBuilder> makeMappedSubnet(
    const SubnetSpace &space,
    const std::shared_ptr<model::SubnetBuilder> oldBuilder) {
  // Maps old entry indices to matches.
  const size_t oldSize = oldBuilder->getMaxIdx() + 1;
  MatchSelection matches(oldSize);

  // Find best coverage by traversing the subnet in reverse order.
  model::SubnetView view(*oldBuilder);
  model::SubnetViewWalker walker(view,
      [&matches](model::SubnetBuilder &,
                 const size_t entryID) -> size_t {
        // Returns the cell arity.
        assert(matches[entryID]);
        return matches[entryID]->links.size();
      },
      [&matches](model::SubnetBuilder &,
                 const size_t entryID,
                 const size_t linkIdx) -> model::Subnet::Link {
        // Returns the corresponding link (cut boundary).
        assert(matches[entryID]);
        return matches[entryID]->links[linkIdx];
      }
  );

  walker.runForward(
      nullptr /* forward visitor */,
      [&space, &matches](model::SubnetBuilder &,
                         bool, bool,
                         const size_t entryID) -> bool {
        assert(!matches[entryID] && space[entryID]->hasSolution());
        matches[entryID] = &space[entryID]->getBest().solution;
        return true;
      }, UTOPIA_TECHMAP_SAVE_MAPPED_POINTS);

  auto newBuilder = std::make_shared<model::SubnetBuilder>();

  // Maps old entry indices to new links.
  std::vector<model::Subnet::Link> links(oldSize);

#if !UTOPIA_TECHMAP_SAVE_MAPPED_POINTS
  // Iterate over all subnet cells and handle the mapped ones.
  for (auto it = oldBuilder->begin(); it != oldBuilder->end(); ++it) {
    const auto entryID = *it;
#else
  // Iterate over the saved (mapped) subnet cells.
  const auto &sequence = walker.getSavedEntries();
  for (size_t s = 0; s < sequence.size(); ++s) {
    const auto entryID = sequence[s].entryID;
#endif // !UTOPIA_TECHMAP_SAVE_MAPPED_POINTS

    const auto &oldCell = oldBuilder->getCell(entryID);

    if (oldCell.isIn()) {
      // Add all inputs even if some of them are not used (matches[i] is null).
      links[entryID] = newBuilder->addInput();
    }
#if !UTOPIA_TECHMAP_SAVE_MAPPED_POINTS
    else if (matches[entryID]) {
#else
    else {
      assert(matches[entryID]);
#endif // !UTOPIA_TECHMAP_SAVE_MAPPED_POINTS

      const auto &newType = model::CellType::get(matches[entryID]->typeID);
      assert(newType.getInNum() == matches[entryID]->links.size());

      model::Subnet::LinkList newLinks(newType.getInNum());
      for (size_t j = 0; j < newType.getInNum(); ++j) {
        const auto oldLink = matches[entryID]->links[j];
        const auto newLink = links[oldLink.idx];
        newLinks[j] = oldLink.inv ? ~newLink : newLink;

        if (!matches[oldLink.idx] && !oldBuilder->getCell(oldLink.idx).isIn()) {
          const auto &oldType = oldCell.getType();

          UTOPIA_ERROR("No match found for "
              << fmt::format("link#{}", j) << " of "
              << fmt::format("cell#{}:{}", entryID, oldType.getName()));
          return nullptr /* error */;
        } 

        if (newType.isCell() && newLinks[j].inv) {
          UTOPIA_ERROR("Invertor (logical gate NOT) "
              << fmt::format("link#{}",  j) << " in "
              << fmt::format("cell#<new>:{}", newType.getName()));
          return nullptr /* error */;
        }
      }
      
      const auto link = newBuilder->addCell(matches[entryID]->typeID, newLinks);
      links[entryID] = matches[entryID]->inversion ? ~link : link;

      const auto isOldOut = oldCell.isOut();
      const auto isNewOut = newType.isOut();

#if !UTOPIA_TECHMAP_MATCH_OUTPUTS
      assert(isOldOut == isNewOut);
#else
      if (isOldOut && !isNewOut) {
        newBuilder->addOutput(link);
      }
#endif // !UTOPIA_TECHMAP_MATCH_OUTPUTS
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

static inline float getProgress(const std::shared_ptr<model::SubnetBuilder> &builder,
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

std::shared_ptr<model::SubnetBuilder> SubnetTechMapper::map(
    const std::shared_ptr<model::SubnetBuilder> &builder) const {
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

#if !UTOPIA_TECHMAP_MATCH_OUTPUTS
      const auto link = builder->getLink(entryID, 0);
      const Match match{model::getCellTypeID(model::OUT), {link}, false};
      space[entryID]->add(match, space[link.idx]->getBest().vector);
      continue;
#endif // !UTOPIA_TECHMAP_MATCH_OUTPUTS
    }

    const auto cuts = cutProvider(*builder, entryID);

    for (const auto &cut : cuts) {
      assert(cut.rootEntryIdx == entryID);

      // Skip trivial and unmapped cuts.
      if (cut.isTrivial() || !hasSolutions(space, cut)) {
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
      UTOPIA_LOG_WARN("No match found for "
          << fmt::format("cell#{}:{}", entryID, cell.getType().getName()));
    }

    if (progress > 0.5 &&  space[entryID]->hasSolution()
                       && !space[entryID]->hasFeasible()) {
      // Do recovery.
      if (tryCount < maxTries) {
        UTOPIA_LOG_COST_VECTOR(
            "Solution is likely not to satisfy the constraints ("
                << static_cast<unsigned>(100 * progress) << "%)",
            space[entryID]->getBest().vector);

        tension *= space[entryID]->getTension();

        UTOPIA_LOG_INFO("Starting the recovery process");
        goto RECOVERY;
      }
    }
  } // for cells

  assert(outputs.size() == builder->getOutNum());
  optimizer::CutExtractor::Cut resultCut(model::OBJ_NULL_ID, outputs);

  if (!hasSolutions(space, resultCut)) {
    UTOPIA_ERROR("Incomplete mapping: "
        "there are cuts that do not match library cells");
    return nullptr /* error */;
  }

  const auto subnetCostVectors = getCostVectors(space, resultCut);
  const auto subnetAggregation = costAggregator(subnetCostVectors);

  const auto isFeasible = criterion.check(subnetAggregation);

  UTOPIA_LOG_COST_VECTOR(
      (isFeasible
          ? "Solution satisfies the constraints"
          : "Solution does not satisfy the constraints"),
      subnetAggregation);
 
  if (!isFeasible && tryCount < maxTries) {
    tension *= criterion.getTension(subnetAggregation);

    UTOPIA_LOG_INFO("Starting the recovery process");
    goto RECOVERY;
  }

  return makeMappedSubnet(space, builder);  
}

} // namespace eda::gate::techmapper
