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

using Subnet           = model::Subnet;
using SubnetBuilder    = model::SubnetBuilder;
using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
using SubnetID         = model::SubnetID;
using CutExtractor     = optimizer::CutExtractor;

techMapperWrapper::techMapResult techMapperWrapper::techMap() {
    
  for (size_t i = 0, size = design_.getSubnetNum(); i < size; ++i) {
    const auto &subnetBuilder = design_.getSubnetBuilder(i);
    const auto techmapBuilder = generateTechSubnet(subnetBuilder);
    if (!techmapBuilder) {
      return techMapResult{false, i};
    }
    design_.setSubnetBuilder(i, techmapBuilder);
  }
  return techMapResult{true};
}

//TODO: a lot of same objects created inside for each function call,
//until threads are implemented they should be moved to techMap()
//function and passed as parameters here
std::shared_ptr<model::SubnetBuilder>
techMapperWrapper::generateTechSubnet(
  const SubnetTechMapperBase::SubnetBuilderPtr &builder) {

  //TODO: should be const ref
  auto &techLibrary = *context_.techMapContext.library;

  // Find cheapest cells and calculate super cells over them.
  techLibrary.prepareLib();

  // Maximum number of cuts per cell
  constexpr uint16_t maxCutNum = 4;

  // Set matcher type (hardcoded to boolMatcher)
  auto pBoolMatcher {Matcher<PBoolMatcher, std::size_t>::create(
                      techLibrary.getCombCells())};

  auto matchFinder = [&](const SubnetBuilderPtr &builder,
                         const optimizer::Cut &cut){
      return pBoolMatcher->match(builder, cut);};

  // Techmapping
  SubnetTechMapperPCut techmapper(
      "SubnetTechMapper",
      context_,
      techLibrary.getProperties().maxArity,
      maxCutNum,
      matchFinder,
      estimator::getPPA);

  auto builderTechmap = techmapper.map(builder);

  if (builderTechmap != nullptr) {
    const auto mappedSubnetID = builderTechmap->make();
    printStatistics(mappedSubnetID, techLibrary);
  }

  return builderTechmap;
}
} // namespace eda::gate::techmapper
