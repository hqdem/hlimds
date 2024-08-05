//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/sdc_manager.h"
#include "gate/model/printer/printer.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techmapper/matcher/func_matcher.h"
#include "gate/techmapper/matcher/matcher.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/translator/graphml.h"
#include "util/env.h"

namespace eda::gate::techmapper {

using ModelPrinter  = model::ModelPrinter;
using Subnet        = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID      = model::SubnetID;
using CutExtractor  = optimizer::CutExtractor;

//TODO
CutExtractor *cutExtractor = nullptr;
FuncMatcher *funcMatcher = nullptr;

static CutExtractor::CutsList cutProvider(
    const SubnetBuilder &builder, const size_t entryID) {
  if (cutExtractor == nullptr) {
    cutExtractor = new CutExtractor(&builder, 6, true);
  }
  return cutExtractor->getCuts(entryID);
}

static std::vector<SubnetTechMapper::Match> matchFinder(
    const SubnetBuilder &builder, const CutExtractor::Cut &cut) {
  return funcMatcher->match(builder, cut);
}

std::shared_ptr<SubnetBuilder> techMap(
    const criterion::Objective objective,
    const std::shared_ptr<SubnetBuilder> &builder) {
  // Set constraints
  criterion::Constraints constraints = {
      criterion::Constraint(criterion::AREA, 10000),   // FIXME:
      criterion::Constraint(criterion::DELAY, 10000),  // FIXME:
      criterion::Constraint(criterion::POWER, 10000)}; // FIXME:
  criterion::Criterion criterion{objective, constraints};

  // Set matcher type
  funcMatcher = Matcher<FuncMatcher, std::size_t>::create(
    library::SCLibrary::get().getCombCells());

  // Techmapping
  SubnetTechMapper *techmapper =
      new SubnetTechMapper("SubnetTechMapper", criterion, cutProvider,
                           matchFinder, estimator::getPPA);
  auto builderTechmap = techmapper->map(builder);
  if (cutExtractor != nullptr) {
    delete cutExtractor;
    cutExtractor = nullptr;
  }
  if (funcMatcher != nullptr) {
    delete funcMatcher;
    funcMatcher = nullptr;
  }

  delete techmapper;

  if (builderTechmap != nullptr) {
    const auto mappedSubnetID = builderTechmap->make();
    printStatistics(mappedSubnetID);
  }

  return builderTechmap;
}
} // namespace eda::gate::techmapper
