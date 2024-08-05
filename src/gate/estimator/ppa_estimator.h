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
#include "gate/library/library_parser.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/subnet_techmapper.h"

#include <algorithm>
#include <cfloat>
#include <unordered_map>

namespace eda::gate::estimator {

using LibraryParser = library::LibraryParser;

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

inline float getArea(const model::CellType &cellType) {
  return cellType.getAttr().props.area;
}

inline float getArea(model::SubnetID subnetID) {
  return processEntries(subnetID, [](const auto &entry) {
    return getArea(entry.cell.getType());
  });
}

inline float getLeakagePower(const model::CellType &cellType) {
    const auto *cell = LibraryParser::get().getLibrary().getCell(
        cellType.getName());
    return cell ?
      cell->getFloatAttribute("cell_leakage_power", FLT_MAX) : 0.0f;
}

inline float getLeakagePower(model::SubnetID subnetID) {
  return processEntries(subnetID, [](const auto &entry) {
    return getLeakagePower(entry.cell.getType());
  });
}

inline float getDelay(const model::CellType &cellType,
               float inputTransTime, float outputCap) {
  return NLDM::delayEstimation(LibraryParser::get().getLibrary(),
     cellType.getName(), inputTransTime, outputCap);
}

inline float getArrivalTime(model::SubnetID subnetID) {
  int timingSense = 0;
  std::unordered_map<uint64_t, float> arrivalMap, delayMap;

  float maxArrivalTime = 0;
  float capacitance = 0;
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
      WLM wlm; // TODO
      float fanoutCap = wlm.getFanoutCap(outNum) + capacitance;
      float slew;
      NLDM::delayEstimation(LibraryParser::get().getLibrary(), entries[i].cell.getType().getName(),
                            delay, fanoutCap, timingSense, slew, delay, capacitance);

      arrivalMap[i] = slew + arrival;
      delayMap[i] = slew;

      maxArrivalTime = std::max(maxArrivalTime, arrivalMap[i]);
    }
    i += entries[i].cell.more;
  }
  return maxArrivalTime;
}

inline criterion::CostVector getPPA(
  const model::CellTypeID cellTypeID, const techmapper::SubnetTechMapper::Context &context) {
  const auto &cellType = model::CellType::get(cellTypeID);
  const auto name = cellType.getName();

  const auto area = getArea(cellType);
  const auto delay = getDelay(cellType, 0, 0); // TODO input transition time and output capacitance should be taken from Context
  const auto power = getLeakagePower(cellType);

  criterion::CostVector result;
  result[criterion::AREA] = area;
  result[criterion::DELAY] = delay;
  result[criterion::POWER] = power;
  return result;
}

} // namespace eda::gate::estimator
