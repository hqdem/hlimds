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

/// Outputs the cost vector.
#define UTOPIA_LOG_COST_VECTOR(prefix, vector)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]  << std::endl\
      << "Delay: " << vector[criterion::DELAY] << std::endl\
      << "Power: " << vector[criterion::POWER] << std::endl\
      << "Total: " << criterion::getIntegralCost(vector) << std::endl)

/// Outputs the tension vector.
#define UTOPIA_LOG_TENSION_VECTOR(prefix, vector)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]  << std::endl\
      << "Delay: " << vector[criterion::DELAY] << std::endl\
      << "Power: " << vector[criterion::POWER] << std::endl)

/// Outputs the cost and tension vectors.
#define UTOPIA_LOG_COST_AND_TENSION_VECTORS(prefix, vector, tension)\
  UTOPIA_LOG_INFO(prefix << std::endl\
      << "Area:  " << vector[criterion::AREA]\
          << " (" << tension[criterion::AREA] << ")" << std::endl\
      << "Delay: " << vector[criterion::DELAY]\
          << " (" << tension[criterion::DELAY] << ")" << std::endl\
      << "Power: " << vector[criterion::POWER]\
          << " (" << tension[criterion::POWER] << ")" << std::endl\
      << "Total: " << criterion::getIntegralCost(vector) << std::endl)
 
namespace eda::gate::techmapper {

static criterion::CostVector defaultCostAggregator(
    const std::vector<criterion::CostVector> &vectors) {
  criterion::CostVector result = criterion::CostVector::Zero;

  for (const auto &vector : vectors) {
    criterion::aggregateCost(result, vector);
  }

  return result;
}

static criterion::CostVector defaultCostPropagator(
    const criterion::CostVector &vector, const uint32_t fanout) {
  const auto divisor = fanout ? fanout : 1;

  return criterion::CostVector(
      vector[criterion::AREA] / divisor /* area flow */,
      vector[criterion::DELAY],
      vector[criterion::POWER] / divisor /* power flow */);
}

SubnetTechMapperBase::SubnetTechMapperBase(
    const std::string &name,
    const context::UtopiaContext &context,
    const CutProvider cutProvider,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperBase(name,
                         context,
                         cutProvider,
                         matchFinder,
                         cellEstimator,
                         defaultCostAggregator,
                         defaultCostPropagator) {}

/// Maps a entry ID to the best match (null stands for no match).
using MatchSelection = std::vector<const SubnetTechMapperBase::Match*>;

/// Find best coverage by traversing the subnet in reverse order.
static void findBestCoverage(
    const SubnetTechMapperBase::SubnetBuilderPtr &oldBuilder,
    const SubnetTechMapperBase::SubnetSpace &space,
    MatchSelection &matches) {
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
      }, false);
} 

static SubnetTechMapperBase::SubnetBuilderPtr makeMappedSubnet(
    const SubnetTechMapperBase::SubnetSpace &space,
    const SubnetTechMapperBase::SubnetBuilderPtr oldBuilder) {
  // Maps old entry indices to matches.
  const uint32_t oldSize = oldBuilder->getMaxIdx() + 1;
  MatchSelection matches(oldSize);

  // Find best coverage by traversing the subnet in reverse order.
  findBestCoverage(oldBuilder, space, matches);

  auto newBuilder = std::make_shared<model::SubnetBuilder>();

  // Maps old entry indices to new links.
  std::vector<model::Subnet::Link> links(oldSize);

  // Iterate over all subnet cells and handle the mapped ones.
  for (auto it = oldBuilder->begin(); it != oldBuilder->end(); ++it) {
    const auto entryID = *it;
    const auto &oldCell = oldBuilder->getCell(entryID);

    if (oldCell.isIn()) {
      // Add all inputs even if some of them are not used (matches[i] is null).
      links[entryID] = newBuilder->addInput();
    } else if (matches[entryID]) {
      const auto &match = *matches[entryID];
 
      const auto &newType = model::CellType::get(match.typeID);
      assert(newType.getInNum() == match.links.size());

      model::Subnet::LinkList newLinks(newType.getInNum());
      for (uint32_t j = 0; j < newType.getInNum(); ++j) {
        const auto oldLink = match.links[j];
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
      
      const auto outs = newBuilder->addMultiOutputCell(match.typeID, newLinks);
      const auto link = outs[match.output];
      links[entryID] = match.inversion ? ~link : link;

      const auto isOldOut = oldCell.isOut();
      const auto isNewOut = newType.isOut();

#if !UTOPIA_TECHMAP_MATCH_OUTPUTS
      assert(isOldOut == isNewOut);
#else
      if (isOldOut && !isNewOut) {
        newBuilder->addOutput(link);
      }
#endif // !UTOPIA_TECHMAP_MATCH_OUTPUTS
    } // not input

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

void SubnetTechMapperBase::findCellSolutions(
    const SubnetBuilderPtr &builder,
    const model::EntryID entryID,
    const optimizer::CutsList &cuts) {
  const auto &cell = builder->getCell(entryID);
  const CellContext cellContext{static_cast<uint16_t>(cell.refcount)};

  for (const auto &cut : cuts) {
    assert(cut.rootID == entryID);
    criterion::CostVector cutAggregation;

    if (cell.isZero() || cell.isOne()) {
      // Constant cut is of zero cost.
      cutAggregation = criterion::CostVector::Zero;
    } else if (!cut.isTrivial() && hasSolutions(cut)) {
      // Aggregate the leaf cost vectors.
      cutAggregation = costAggregator(getCostVectors(cut)); 
    } else {
      // Skip trivial and unmapped cuts.
      continue;
    }

    if (!context.criterion->check(cutAggregation)) {
      continue;
    }

    // Trivial cuts are treated as constants.
    const auto &matches = getMatches(*builder, cut);

    for (const auto &match : matches) {
      assert((cut.isTrivial() && match.links.empty())
          || (match.links.size() == cut.size()));

      const auto cellCostVector = cellEstimator(
          match.typeID, cellContext, context.techMapContext);
      const auto costVector = cutAggregation + cellCostVector;

      if (!context.criterion->check(costVector)) {
        continue;
      }

      space[entryID]->add(match, costPropagator(costVector, cell.refcount));
    } // for matches
  } // for cuts
}

SubnetTechMapperBase::Status SubnetTechMapperBase::techMap(
    const SubnetBuilderPtr &builder) {
  // Subnet outputs to be filled in the loop below.
  std::unordered_set<model::EntryID> outputs;
  outputs.reserve(builder->getOutNum());

  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto entryID = *it;
    const auto &cell = builder->getCell(entryID);

    // Must be called for all entries (even for inputs).
    const auto cuts = cutProvider(*builder, entryID);
    assert(!cuts.empty());

    // Handle the input and constant cells.
    if (cell.isIn() ||
        (!enableConstMapping && (cell.isZero() || cell.isOne()))) {
      const Match match{cell.getTypeID(), {}, false};
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

    findCellSolutions(builder, entryID, cuts);

    if (!space[entryID]->hasSolution()) {
      UTOPIA_LOG_WARN("No match found for "
          << fmt::format("cell#{}:{}", entryID, cell.getType().getName()));
    }
  } // for cells

  assert(outputs.size() == builder->getOutNum());
  optimizer::Cut resultCut(outputs.size(), model::OBJ_NULL_ID, outputs, true);

  if (!hasSolutions(resultCut)) {
    return Status{Status::UNSAT};
  }

  const auto subnetCostVectors = getCostVectors(resultCut);
  const auto subnetAggregation = costAggregator(subnetCostVectors);

  const auto isFeasible = context.criterion->check(subnetAggregation);
  const auto subnetTension = context.criterion->getTension(subnetAggregation);

  return Status{Status::FOUND, isFeasible, subnetAggregation, subnetTension};
}

SubnetTechMapperBase::SubnetBuilderPtr SubnetTechMapperBase::map(
    const SubnetBuilderPtr &builder) const {
  auto *thisPtr = const_cast<SubnetTechMapperBase *>(this);
  thisPtr->onBegin(builder);

  SubnetBuilderPtr result = nullptr;

  while (tryCount < maxTries) {
    const auto finalTry = (tryCount == maxTries - 1);

    // Do technology mapping for the given criterion and tension.
    const auto status = thisPtr->techMap(builder);

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
          "Solution does not satisfy the constraints",
          status.vector, status.tension);
    }

    if (!thisPtr->onRecovery(builder, status)) break;
    UTOPIA_LOG_TENSION_VECTOR(
        "Starting the recovery process w/ direction", tension);
  }

  if (!result) {
    UTOPIA_ERROR(
        "Incomplete mapping: there are cuts that do not match library cells");
  }

  thisPtr->onEnd(result);
  return result;
}

void SubnetTechMapperBase::onBegin(const SubnetBuilderPtr &oldBuilder) {
  tryCount = 0;

  // No penalties at the beginning.
  tension = criterion::CostVector::Zero;

  space.resize(oldBuilder->getMaxIdx() + 1);
  for (auto i = oldBuilder->begin(); i != oldBuilder->end(); i.nextCell()) {
    space[*i] = std::make_unique<CellSpace>(*context.criterion, tension);
  }
}

bool SubnetTechMapperBase::onRecovery(const SubnetBuilderPtr &oldBuilder,
                                      const Status &status) {
  tryCount++;

  // If no chance, break the technology mapping.
  if (status.verdict == Status::UNSAT) {
    return false;
  }

  const auto &unit = criterion::CostVector::Unit;

  if (tryCount == 1) {
    // Sharpen the initial tension vector.
    const auto softmax = status.tension.softmax(.1 /* temperature */);
    tension = softmax * (status.tension.norm(2.) / softmax.norm(2.));
  } else {
    // Modify the tension vector according to the current result.
    constexpr criterion::Cost inflation = 1.01;
    tension *= status.tension.smooth(unit, 0.5) * inflation;
  }

  for (auto i = oldBuilder->begin(); i != oldBuilder->end(); i.nextCell()) {
    const auto &local = space[*i];
    local->reset(tension);
  }

  return true;
}

} // namespace eda::gate::techmapper
