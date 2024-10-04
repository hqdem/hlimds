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

using CellContext = techmapper::SubnetTechMapperBase::CellContext;

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

inline double getEstimatedOutCapacitance(const library::StandardCell &cell,
                                         const library::WireLoadModel *wlm,
                                         size_t fanout) {
  if (fanout == 0) {
    return 0.0;
  }
  double cellCap = NLDM::cellOutputCapEstimation(cell, fanout);
  double fanoutCap = (wlm == nullptr) ? cellCap :
                      cellCap + wlm->getFanoutCapacitance(fanout);
  return fanoutCap;
}

inline double getDelay(const library::StandardCell &cell,
                       const CellContext &cellContext,
                       const library::SCLibrary &library) {
  const auto maxInputByDelay = std::max_element(
    cellContext.inputs.begin(),
    cellContext.inputs.end(),
    [](const CellContext::LinkedInputCell &a,
        const CellContext::LinkedInputCell &b) {
          return (*a.costs)[criterion::DELAY] < (*b.costs)[criterion::DELAY];
    });
  //TODO: just taking maximum input delay for now. Should be smarter about this
  double inputDelay = maxInputByDelay != cellContext.inputs.end() ?
                      (*maxInputByDelay->costs)[criterion::DELAY] :
                      0.0;
  // TODO: properly check that deafult wlm is set
  const library::WireLoadModel *wlm = library.getProperties().defaultWLM;
  double fanoutCap = getEstimatedOutCapacitance(cell, wlm, cellContext.fanout);
  auto estimatedSDC = NLDM::cellOutputSDEstimation(cell, inputDelay, fanoutCap);
  return estimatedSDC.delay;
}

inline double getArrivalTime(model::SubnetID subnetID,
                             const library::SCLibrary &library) {
  std::unordered_map<uint64_t, double> arrivalMap, delayMap;

  double maxArrivalTime = 0;
  // TODO: properly check that deafult wlm is set
  const library::WireLoadModel *wlm = library.getProperties().defaultWLM;

  auto entries = model::Subnet::get(subnetID).getEntries();
  for (uint64_t i = 0; i < entries.size(); i += 1 + entries[i].cell.more) {
    if (shouldSkipCell(entries[i].cell)) {
      continue;
    }
    double delay = 0, arrival = 0;
    for (const auto& link : entries[i].cell.link) {
      if (delayMap.count(link.idx)) {
        delay = std::max(delay, delayMap[link.idx]);
        arrival = std::max(arrival, arrivalMap[link.idx]);
      }
    }
    const auto *cellPtr = library.getCellPtr(entries[i].cell.getTypeID());
    assert(cellPtr != nullptr);

    size_t fanout = entries[i].cell.getOutNum();
    
    /// TODO: can do proper calculations instead of estimation
    double fanoutCap = getEstimatedOutCapacitance(*cellPtr, wlm, fanout);
    auto estimatedSD = NLDM::cellOutputSDEstimation(*cellPtr, delay,
                                                      fanoutCap);

    arrivalMap[i] = estimatedSD.slew + arrival;
    delayMap[i] = estimatedSD.delay + delay;

    maxArrivalTime = std::max(maxArrivalTime, delayMap[i]);
  }
  return maxArrivalTime;
}

inline criterion::CostVector getPPA(
    const model::CellTypeID cellTypeID,
    const CellContext &cellContext,
    const context::TechMapContext &techmapContext) {
  const auto &cellType = model::CellType::get(cellTypeID);
  const auto name = cellType.getName();

  const auto *cellPtr = techmapContext.library->getCellPtr(cellTypeID);
  if (cellPtr == nullptr) {
    assert(false && "calling getPPa for nonexistent CellTypeID");
  }

  const auto area = getArea(cellType);
  const auto delay = getDelay(*cellPtr, cellContext, *techmapContext.library);
  const auto power = getLeakagePower(*cellPtr);

  return criterion::CostVector(area, delay, power);
}

} // namespace eda::gate::estimator
