//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/cut_based/delay_estmt/delay_estmt.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_delay/simple_delay_mapper.h"
#include "gate/techmapper/library/liberty_manager.h"

#include <readcells/groups.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <limits>

namespace eda::gate::techmapper {

using Subnet = eda::gate::model::Subnet;
using SubnetID = eda::gate::model::SubnetID;

void SimpleDelayMapper::findBest() {
  Subnet &subnet = Subnet::get(subnetID);

  for (uint16_t i = 0; i < subnet.getInNum(); i++) {
    delayVec[i] = BestReplacementDelay{0};
  }

  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!(cell.isAnd() || cell.isBuf())) {
      addNotAnAndToTheMap(entryIndex, cell);
    } else {
      // Save best tech cells subnet to bestReplMap
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex));
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
    EntryIndex entryIndex,
    const optimizer::CutExtractor::CutsList &cutsList) {
  eda::gate::optimizer::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestReplacement bestSimpleReplacement{};
  float bestArrivalTime = MAXFLOAT;

  DelayEstimator d1(LibraryManager::get().getLibrary());
  int timingSense = d1.nldm.getSense();

  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB->getSubnetIDsByTT(truthTable.at(0))) {
        auto currentAttr = cellDB->getSubnetAttrBySubnetID(currentSubnetID);

        float inputNetTransition = findMaxArrivalTime(cut.entryIdxs);
        float fanoutCap = d1.wlm.getFanoutCap(currentAttr.fanout_count) + 
                          d1.nldm.getCellCap();

        d1.nldm.delayEstimation(currentAttr.name,
                                inputNetTransition,
                                fanoutCap,
                                timingSense);

        float arrivalTime = d1.nldm.getSlew();

        if (arrivalTime < bestArrivalTime) {
          bestArrivalTime = arrivalTime;
          bestSimpleReplacement.setSubnetID(currentSubnetID);
          bestSimpleReplacement.inputs.clear();
          for (const auto &in : cut.entryIdxs) {
            bestSimpleReplacement.inputs.push_back(in);
          }
        }
        delayVec[entryIndex] = {arrivalTime};
      }
    }
  }

  assert(!bestSimpleReplacement.inputs.empty());
  (*bestReplacementMap)[entryIndex] = bestSimpleReplacement;
}
} // namespace eda::gate::techmapper
