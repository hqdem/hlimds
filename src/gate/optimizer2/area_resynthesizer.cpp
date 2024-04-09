//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/area_resynthesizer.h"

namespace eda::gate::optimizer2 {

using SubnetID = eda::gate::model::SubnetID;

// Heuristic constants for optimizers calling.
const size_t akersMaxInputNum = 8;
const size_t akersArityNum    = 3;
const size_t careCutSize      = 16;

SubnetID AreaResynthesizer::resynthesize(const SubnetFragment &sf) const {
  const auto &subnet = Subnet::get(sf.subnetID);
  const size_t nIn = subnet.getInNum();

  std::vector<size_t> roots;
  for (size_t i = 0; i < nIn; ++i) {
    roots.push_back(sf.entryMap.at(i));
  }

  std::unordered_map<size_t, size_t> map;
  SubnetID careSubnetId = getReconvCone(subnetBuilder, roots, careCutSize, map);

  const auto &careSubnet = Subnet::get(careSubnetId);
  auto care = gate::model::computeCare(careSubnet);
  auto truthTable = gate::model::evaluateSingleOut(subnet);

  SubnetID resynthesizedId = model::OBJ_NULL_ID;
  if ((nIn < akersMaxInputNum) && (maxArity == akersArityNum)) {
    synthesis::AkersSynthesizer akers;
    resynthesizedId = akers.synthesize(truthTable, care);
  } else {
    synthesis::MMSynthesizer isop;
    resynthesizedId = isop.synthesizeWithFactoring(truthTable, care, maxArity);
  }

  if (resynthesizedId == model::OBJ_NULL_ID) {
    return resynthesizedId;
  }

  const auto &resynthesized = Subnet::get(resynthesizedId);
  auto resynthesizedTT = gate::model::evaluateSingleOut(resynthesized);
  resynthesizedTT &= care;
  truthTable &= care;
  assert(resynthesizedTT == truthTable && "Resynthesized TT != Input TT\n");
  return resynthesizedId;
}

SubnetID AreaResynthesizer::resynthesize(SubnetID subnetId) const {
  const auto &subnet = Subnet::get(subnetId);
  const auto truthTable = gate::model::evaluateSingleOut(subnet);
  const auto numVars = truthTable.num_vars();

  SubnetID resynthesizedId = model::OBJ_NULL_ID;
  if ((numVars < akersMaxInputNum) && (maxArity == akersArityNum)) {
    synthesis::AkersSynthesizer akers;
    resynthesizedId = akers.synthesize(truthTable);
  } else {
    synthesis::MMSynthesizer isop;
    resynthesizedId = isop.synthesizeWithFactoring(truthTable, maxArity);
  }

  return resynthesizedId;
}

} // namespace eda::gate::optimizer2
