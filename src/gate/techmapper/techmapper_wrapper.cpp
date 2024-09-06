//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/sdc_manager.h"
#include "gate/premapper/cell_aigmapper.h"
#include "gate/techmapper/matcher/pbool_matcher.h"
#include "gate/techmapper/subnet_techmapper_pcut.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/translator/graphml.h"
#include "util/env.h"

namespace eda::gate::techmapper {

using Subnet        = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID      = model::SubnetID;
using CutExtractor  = optimizer::CutExtractor;

PBoolMatcher *boolMatcher = nullptr;

static std::vector<SubnetTechMapperBase::Match> matchFinder(
    const SubnetBuilder &builder, const optimizer::Cut &cut) {
  return boolMatcher->match(builder, cut);
}

std::shared_ptr<SubnetBuilder> techMap(
    const criterion::Objective objective,
    const std::shared_ptr<SubnetBuilder> &builder) {
  // Set constraints
  criterion::Constraints constraints = {
      criterion::Constraint(criterion::AREA,  100000),   // FIXME:
      criterion::Constraint(criterion::DELAY, 10),       // FIXME:
      criterion::Constraint(criterion::POWER, 100000)};  // FIXME:
  criterion::Criterion criterion{objective, constraints};

  // Maximum number of cuts per cell
  constexpr uint16_t maxCutNum = 8;

  // Set matcher type
  boolMatcher = Matcher<PBoolMatcher, std::size_t>::create(
    library::library->getCombCells());

  // Techmapping
  auto *techmapper = new SubnetTechMapperPCut(
      "SubnetTechMapper",
      criterion,
      library::library->getMaxArity(),
      maxCutNum,
      matchFinder,
      estimator::getPPA);

  auto builderTechmap = techmapper->map(builder);

  if (boolMatcher != nullptr) {
    delete boolMatcher;
    boolMatcher = nullptr;
  }

  delete techmapper;

  if (builderTechmap != nullptr) {
    const auto mappedSubnetID = builderTechmap->make();
    printStatistics(mappedSubnetID);
  }

  return builderTechmap;
}
} // namespace eda::gate::techmapper
