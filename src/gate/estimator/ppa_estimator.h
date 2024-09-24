//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/criterion.h"
#include "gate/estimator/simple_time_model.h"
#include "gate/library/library.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/subnet_techmapper_base.h"

#include <algorithm>
#include <cfloat>
#include <unordered_map>

namespace eda::gate::estimator {

inline bool shouldSkipCell(const model::Subnet::Cell &cell) {
  return cell.isIn() || cell.isOut() || cell.isOne() || cell.isZero();
}

//TODO: probably should be in another header file
template <typename Result, typename Func>
inline Result sumFromSubnetEntriesCells(model::SubnetID subnetID, Func func) {
  Result result {};
  const auto &entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    if (!shouldSkipCell(cell)) {
      result += func(cell);
    }
    i += cell.more;
  }
  return result;
}

inline double getArea(const model::CellType &cellType) {
  return cellType.getAttr().getPhysProps().area;
}

inline double getArea(model::SubnetID subnetID) {
  return sumFromSubnetEntriesCells<double>(subnetID, 
          [](const auto &cell) { return getArea(cell.getType());});
}

inline double getLeakagePower(const library::StandardCell &cell) {
    //const auto *cell = library.getLibrary().getCell(cellType.getName());
    auto leakagePower = cell.propertyLeakagePower;
    assert(!std::isnan(leakagePower));
    return leakagePower;
}

inline double getLeakagePower(model::SubnetID subnetID,
                              const library::SCLibrary &library) {
  return sumFromSubnetEntriesCells<double>(subnetID,
          [&](const auto &cell) {
            const auto *techCell = library.getCellPtr(cell.getTypeID());
            assert(techCell != nullptr);
            return getLeakagePower(*techCell); });
}

inline double getDelay(const library::StandardCell &cell,
                       double inputTransTime, double outputCap) {
  return NLDM::delayEstimation(cell, inputTransTime, outputCap);
}

inline double getArrivalTime(model::SubnetID subnetID,
                            const library::SCLibrary &library) {
  int timingSense = 0;
  std::unordered_map<uint64_t, double> arrivalMap, delayMap;

  double maxArrivalTime = 0;
  double capacitance = 0;
  auto entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); ++i) {
    if (!shouldSkipCell(entries[i].cell)) {
      double delay = 0, arrival = 0;
      for (const auto& link : entries[i].cell.link) {
        if (delayMap.count(link.idx)) {
          delay = std::max(delay, delayMap[link.idx]);
          arrival = std::max(arrival, arrivalMap[link.idx]);
        }
      }

      size_t outNum = entries[i].cell.getOutNum();
      WLM wlm; // TODO
      double fanoutCap = wlm.getFanoutCap(outNum) + capacitance;
      double slew;

      const auto *cellPtr = library.getCellPtr(entries[i].cell.getTypeID());
      assert(cellPtr != nullptr);
      NLDM::delayEstimation(*cellPtr, delay, fanoutCap,
                            timingSense, slew, delay, capacitance);

      arrivalMap[i] = slew + arrival;
      delayMap[i] = slew;

      maxArrivalTime = std::max(maxArrivalTime, arrivalMap[i]);
    }
    i += entries[i].cell.more;
  }
  return maxArrivalTime;
}

inline criterion::CostVector getPPA(
    const model::CellTypeID cellTypeID,
    const techmapper::SubnetTechMapperBase::CellContext &cellContext,
    const context::TechMapContext &techmapContext) {
  const auto &cellType = model::CellType::get(cellTypeID);
  const auto name = cellType.getName();

  const auto *cellPtr = techmapContext.library->getCellPtr(cellTypeID);
  if (cellPtr == nullptr) {
    assert(false && "calling getPPa for nonexistent CellTypeID");
  }
  const auto area = getArea(cellType);
  const auto delay = getDelay(*cellPtr, 0, 0); // TODO input transition time and output capacitance should be taken from Context
  const auto power = getLeakagePower(*cellPtr);

  return criterion::CostVector(area, delay, power);
}

} // namespace eda::gate::estimator
