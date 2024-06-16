//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simple_time_model.h"
#include "gate/library/liberty_manager.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/func_mapper/simple_delay/simple_delay_mapper.h"

#include <readcells/groups.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <limits>

namespace eda::gate::techmapper {

using LibertyManager = library::LibertyManager;
using SCLibrary = library::SCLibrary;
using Subnet = model::Subnet;
using SubnetID = model::SubnetID;

void SimpleDelayMapper::map(
       const SubnetID subnetID, const SCLibrary &cellDB,
       const SDC &sdc, Mapping &mapping) {
  this->subnetID = subnetID;
  cutExtractor = new optimizer::CutExtractor(&model::Subnet::get(subnetID), 6);
  Subnet &subnet = Subnet::get(subnetID);

  for (uint16_t i = 0; i < subnet.getInNum(); i++) {
    delayVec[i] = BestReplacementDelay{0};
  }

  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!(cell.isAnd() || cell.isBuf())) {
      addNotAnAndToTheMap(entryIndex, cell, mapping);
    } else {
      // Save best tech cells subnet to bestReplMap
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex), cellDB, mapping);
    }
    entryIndex += cell.more;
  }
  delayVec.clear();
}


float SimpleDelayMapper::findMaxArrivalTime(const std::unordered_set<size_t> &entryIdxs){
  float maxArrivalTime = std::numeric_limits<float>::lowest();

  for (const auto& idx : entryIdxs) {
    auto it = delayVec.find(idx);
    if (it != delayVec.end()) {
      maxArrivalTime = std::max(maxArrivalTime, it->second.arrivalTime);
    }
  }

  return maxArrivalTime;
}

void SimpleDelayMapper::saveBest(
    const EntryIndex entryIndex,
    const optimizer::CutExtractor::CutsList &cutsList,
    const SCLibrary &cellDB, Mapping &mapping) {
  eda::gate::optimizer::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  MappingItem mappingItem;
  float bestArrivalTime = MAXFLOAT;

  estimator::WLM wlm;
  int timingSense = 0;

  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));
      float capacitance = 0;

      for (const SubnetID &currentSubnetID : cellDB.getSubnetID(truthTable.at(0))) {
        auto currentAttr = cellDB.getCellAttrs(currentSubnetID);

        float inputNetTransition = findMaxArrivalTime(cut.entryIdxs);
        float fanoutCap = wlm.getFanoutCap(currentAttr.fanoutCount) + capacitance;
        float slew = 0; // TODO
        float delay = 0; // TODO
        estimator::NLDM::delayEstimation(LibertyManager::get().getLibrary(), currentAttr.name,
          inputNetTransition, fanoutCap, timingSense, slew, delay, capacitance);

        if (slew < bestArrivalTime) {
          bestArrivalTime = slew;
          mappingItem.setSubnetID(currentSubnetID);
          mappingItem.inputs.clear();
          for (const auto &in : cut.entryIdxs) {
            mappingItem.inputs.push_back(in);
          }
        }
        delayVec[entryIndex] = {slew};
      }
    }
  }

  assert(!mappingItem.inputs.empty());
  mapping[entryIndex] = mappingItem;
}
} // namespace eda::gate::techmapper
