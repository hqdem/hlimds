//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/model/subnetview.h"
#include "gate/techmapper/subnet_techmapper_base.h"

#include <cassert>
#include <memory>
#include <queue>
#include <unordered_set>

/// Disables cut extraction and matching for outputs.
#define UTOPIA_TECHMAP_MATCH_OUTPUTS 1

/// Saves mapped subnet points when selecting best matches.
#define UTOPIA_TECHMAP_SAVE_MAPPED_POINTS 1

/// Outputs the cost vector.
#define UTOPIA_LOG_VECTOR(prefix, vector)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]  << std::endl\
      << "Delay: " << vector[criterion::DELAY] << std::endl\
      << "Power: " << vector[criterion::POWER] << std::endl)

/// Outputs the cost vector.
#define UTOPIA_LOG_COST_AND_TENSION_VECTORS(prefix, vector, tension)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]\
          << " (" << tension[criterion::AREA] << ")" << std::endl\
      << "Delay: " << vector[criterion::DELAY]\
          << " (" << tension[criterion::DELAY] << ")" << std::endl\
      << "Power: " << vector[criterion::POWER]\
          << " (" << tension[criterion::POWER] << ")" << std::endl)
 
namespace eda::gate::techmapper {

static criterion::CostVector defaultCostAggregator(
    const std::vector<criterion::CostVector> &vectors) {
  criterion::CostVector result = criterion::CostVector::Zero;

  for (uint16_t i = 0; i < vectors.size(); ++i) {
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
  // Area flow heuristic.
  result[criterion::AREA]  = vector[criterion::AREA] / divisor;
  // Time delay propagation.
  result[criterion::DELAY] = vector[criterion::DELAY];
  // Power flow heuristic.
  result[criterion::POWER] = vector[criterion::POWER] / divisor;

  return result;
}

SubnetTechMapperBase::SubnetTechMapperBase(
    const std::string &name,
    const criterion::Criterion &criterion,
    const CutProvider cutProvider,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperBase(name,
                         criterion,
                         cutProvider,
                         matchFinder,
                         cellEstimator,
                         defaultCostAggregator,
                         defaultCostPropagator) {}

/// Maps a entry ID to the best match (null stands for no match).
using MatchSelection = std::vector<const SubnetTechMapperBase::Match*>;

static SubnetTechMapperBase::SubnetBuilderPtr makeMappedSubnet(
    const SubnetTechMapperBase::SubnetSpace &space,
    const SubnetTechMapperBase::SubnetBuilderPtr oldBuilder) {
  // Maps old entry indices to matches.
  const uint32_t oldSize = oldBuilder->getMaxIdx() + 1;
  MatchSelection matches(oldSize);

  // Find best coverage by traversing the subnet in reverse order.
  model::SubnetView view(*oldBuilder);
  model::SubnetViewWalker walker(view,
      [&matches](model::SubnetBuilder &,
                 const uint32_t entryID) -> uint16_t {
        // Returns the cell arity.
        assert(matches[entryID]);
        return matches[entryID]->links.size();
      },
      [&matches](model::SubnetBuilder &,
                 const uint32_t entryID,
                 const uint16_t linkIdx) -> model::Subnet::Link {
        // Returns the corresponding link (cut boundary).
        assert(matches[entryID]);
        return matches[entryID]->links[linkIdx];
      }
  );

  walker.runForward(
      nullptr /* forward visitor */,
      [&space, &matches](model::SubnetBuilder &,
                         bool, bool,
                         const uint32_t entryID) -> bool {
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
      for (uint32_t j = 0; j < newType.getInNum(); ++j) {
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

static inline float getProgress(
    const SubnetTechMapperBase::SubnetBuilderPtr &builder,
    const uint32_t count) {
  const auto nIn = builder->getInNum();
  const auto nOut = builder->getOutNum();
  const auto nAll = builder->getCellNum();

  // Ignore the subnet inputs.
  if (count < nIn) { return 0.; }

  // Last non-output cell index.
  const auto k = nAll - nOut - 1;
  const auto j = (count > k) ? k : count;

  // Progress reaches 100% when merging outputs.
  return static_cast<float>(j + 1 - nIn) /
         static_cast<float>(nAll - nIn);
}

SubnetTechMapperBase::Status SubnetTechMapperBase::map(
    const SubnetBuilderPtr &builder,
    const bool enableEarlyRecovery) {
  // Subnet outputs to be filled in the loop below.
  std::unordered_set<model::EntryID> outputs;
  outputs.reserve(builder->getOutNum());

  uint32_t cellCount{0};
  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto progress = getProgress(builder, cellCount++);
    assert(0. <= progress && progress <= 1.);

    const auto entryID = *it;
    const auto &cell = builder->getCell(entryID);

    // Must be called for all entries (even for inputs).
    const auto cuts = cutProvider(*builder, entryID);
    assert(!cuts.empty());

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

    for (const auto &cut : cuts) {
      assert(cut.rootID == entryID);

      // Skip trivial and unmapped cuts.
      if (cut.isTrivial() || !hasSolutions(cut)) {
        continue;
      }

      const auto cutCostVectors = getCostVectors(cut);
      const auto cutAggregation = costAggregator(cutCostVectors);

      if (!criterion.check(cutAggregation)) {
        continue;
      }

      const auto &matches = getMatches(*builder, cut);

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

    if (enableEarlyRecovery && progress > 0.5 &&
        space[entryID]->hasSolution() &&
       !space[entryID]->hasFeasible()) {
      const auto &partialVector = space[entryID]->getBest().vector;
      const auto &partialTension = space[entryID]->getTension();
      return Status{Status::RERUN, progress, partialVector, partialTension};
    }
  } // for cells

  assert(outputs.size() == builder->getOutNum());
  optimizer::Cut resultCut(outputs.size(), model::OBJ_NULL_ID, outputs, true);

  if (!hasSolutions(resultCut)) {
    return Status{Status::UNSAT};
  }

  const auto subnetCostVectors = getCostVectors(resultCut);
  const auto subnetAggregation = costAggregator(subnetCostVectors);

  const auto isFeasible = criterion.check(subnetAggregation);
  const auto subnetTension = criterion.getTension(subnetAggregation);

  return Status{Status::FOUND, isFeasible, subnetAggregation, subnetTension};
}

SubnetTechMapperBase::SubnetBuilderPtr SubnetTechMapperBase::map(
    const SubnetBuilderPtr &builder) const {
  auto *thisPtr = const_cast<SubnetTechMapperBase *>(this);
  thisPtr->onBegin(builder);

  SubnetBuilderPtr result = nullptr;

  while (tryCount < maxTries) {
    const auto finalTry = (tryCount == maxTries - 1);

    // Do technology mapping.
    const auto status = thisPtr->map(builder, !finalTry);

    if (status.verdict == Status::FOUND && (status.isFeasible || finalTry)) {
      UTOPIA_LOG_COST_AND_TENSION_VECTORS(
          (status.isFeasible
              ? "Solution satisfies the constraints"
              : "Solution does not satisfy the constraints"),
           status.vector, status.tension);
      result = makeMappedSubnet(space, builder);
      break;
    }

    if (status.verdict != Status::UNSAT) {
      UTOPIA_LOG_COST_AND_TENSION_VECTORS(
          "Solution is likely not to satisfy the constraints ("
              << static_cast<unsigned>(100. * status.progress) << "%)",
          status.vector, status.tension);
    }

    if (!thisPtr->onRecovery(status)) break;
    UTOPIA_LOG_VECTOR("Starting the recovery process w/ direction", tension);
  }

  if (!result) {
    UTOPIA_ERROR("Incomplete mapping: "
        "there are cuts that do not match library cells");
  }

  thisPtr->onEnd(result);
  return result;
}

} // namespace eda::gate::techmapper
