//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include <assert.h>

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techoptimizer/mapper/cut_base/simple_delay/simple_delay_mapper.h"
#include "gate/techoptimizer/mapper/cut_base/delay_estmt/delay_estmt.h"

#include <algorithm>
#include <limits>

namespace eda::gate::tech_optimizer {
void SimpleDelayMapper::findBest() {
  auto startFB = std::chrono::high_resolution_clock::now();
  std::cout << "Finding best tech cell for every cut" << std::endl;
  Subnet &subnet = Subnet::get(subnetID);

  for (uint16_t i = 0; i < subnet.getInNum(); i++) {
    delayVec[i] = BestReplacementDelay{0};
  }

  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!cell.isAnd()) {
      addNotAnAndToTheMap(entryIndex, cell);
    } else {
      // Save best tech cells subnet to bestReplMap
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex));
    }
    entryIndex += cell.more;
  }
  auto endFB = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> FBTime = endFB - startFB;
  std::cout << "Функция Finding best tech cell выполнялась " << FBTime.count()
      << " секунд.\n";
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
    const optimizer2::CutExtractor::CutsList &cutsList) {
  eda::gate::optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestReplacement bestSimpleReplacement{};
  float bestArrivalTime = MAXFLOAT;
  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.size() != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB->getSubnetIDsByTT(truthTable.at(0))) {
        auto currentAttr = cellDB->getSubnetAttrBySubnetID(currentSubnetID);

        delay_estimation::DelayEstimator d1;

        std::string file = "test/data/gate/tech_mapper/sky130_fd_sc_hd__ff_100C_1v65.lib";
        float inputNetTransition = findMaxArrivalTime(cut.entryIdxs);
        float fanoutCount = d1.wlm.getFanoutCap(currentAttr.fanout_count);

        d1.nldm.delayEstimation(currentAttr.name,
                                file,
                                inputNetTransition,
                                fanoutCount);

        float arrivalTime = d1.nldm.getSlew();

        if (arrivalTime < bestArrivalTime) {
          bestArrivalTime = arrivalTime;
          bestSimpleReplacement.subnetID = currentSubnetID;
          bestSimpleReplacement.entryIDxs = cut.entryIdxs;
        }
        delayVec[entryIndex] = {arrivalTime};
      }
    }
  }

  assert(!bestSimpleReplacement.entryIDxs.empty());
  (*bestReplacementMap)[entryIndex] = bestSimpleReplacement;
}
} // namespace eda::gate::tech_optimizer