//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/techmapper/comb_mapper/func_mapper/delay_estmt/delay_estmt.h"

#include <algorithm>
#include <unordered_map>

namespace eda::gate::techmapper {

inline bool shouldSkipCell(const model::Subnet::Cell &cell) {
  return cell.isIn() || cell.isOut() || cell.isOne() || cell.isZero();
}

template <typename Func>
inline float processEntries(model::SubnetID subnetID, Func func) {
  float result = 0;
  auto entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); ++i) {
    if (!shouldSkipCell(entries[i].cell)) {
      result += func(entries[i]);
    }
    i += entries[i].cell.more;
  }
  return result;
}

inline float getArea(model::SubnetID subnetID) {
  return processEntries(subnetID, [](const auto &entry) {
    return entry.cell.getType().getAttr().props.area;
  });
}

inline float getLeakagePower(model::SubnetID subnetID) {
  return processEntries(subnetID, [](const auto &entry) {
    const auto *cell = LibertyManager::get().getLibrary().getCell(
        entry.cell.getType().getName());
    return cell ? cell->getFloatAttribute("cell_leakage_power",
                                          MAXFLOAT) : 0.0f;
  });
}

inline float getArrivalTime(model::SubnetID subnetID) {
  DelayEstimator delayEstimator(LibertyManager::get().getLibrary());
  int timingSense = delayEstimator.nldm.getSense();
  std::unordered_map<uint64_t, float> arrivalMap, delayMap;

  float maxArrivalTime = 0;
  auto entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); ++i) {
    if (!shouldSkipCell(entries[i].cell)) {
      float delay = 0, arrival = 0;
      for (const auto& link : entries[i].cell.link) {
        if (delayMap.count(link.idx)) {
          delay = std::max(delay, delayMap[link.idx]);
          arrival = std::max(arrival, arrivalMap[link.idx]);
        }
      }

      size_t outNum = entries[i].cell.getOutNum();
      float fanoutCap = delayEstimator.wlm.getFanoutCap(outNum) +
          delayEstimator.nldm.getCellCap();
      delayEstimator.nldm.delayEstimation(entries[i].cell.getType().getName(),
                                          delay, fanoutCap, timingSense);

      float arrivalTime = delayEstimator.nldm.getSlew();
      arrivalMap[i] = arrivalTime + arrival;
      delayMap[i] = arrivalTime;

      maxArrivalTime = std::max(maxArrivalTime, arrivalMap[i]);
    }
    i += entries[i].cell.more;
  }
  return maxArrivalTime;
}

} // namespace eda::gate::techmapper