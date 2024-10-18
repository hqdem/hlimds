//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simple_time_model.h"
#include "util/double_math.h"
#include "util/env.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::estimator {

using namespace eda::util::doubleMath;

//return lower and uper iterators
//if lower==end, then all elements are lesser than targetVal, {0,1} returned
//if upper==begin, then all elements are greater than targetVal,
// {end()-2, end()-1} returned
static inline std::pair<double, double>
getInterpolationIndexes(const std::vector<double> &lutIndex, double targetVal) {
  assert(lutIndex.size() >= 2); //TODO: need to handle this case
  constexpr auto prescision = EPSDOUBLE * 10;
  ssize_t lowID = -1, upID = -1;
  auto doubleComp = [](double source, double value) {
                      return source < value &&
                      !eqvDouble(source, value, prescision);
                    };
  auto lower = std::lower_bound(lutIndex.begin(), lutIndex.end(),
                                targetVal, doubleComp);
  auto upper = std::upper_bound(lutIndex.begin(), lutIndex.end(),
                                targetVal, doubleComp);

  if (upper == lutIndex.begin()) { //targetVal < lutIndex[0]
    if (eqvDouble(*upper, targetVal, prescision)) { //targetVal == lutIndex[0]
      lowID = 0; upID = 0;
    } else {
      lowID = 0; upID = 1;
    }
  } else if (upper != lutIndex.end()) { // *lower < targetVal < *upper
    if (eqvDouble(*lower, targetVal, prescision)) { // targetVal == *lower
        lowID = std::distance(lutIndex.begin(), lower);
        upID = lowID;
    } else if (eqvDouble(*upper, targetVal, prescision)) { // targetVal==*upper
        //TODO: this branch might be dead code
        lowID = std::distance(lutIndex.begin(), upper);
        upID = lowID;
    } else { // *lower < targetVal < *upper
      lowID = (lower == lutIndex.begin()) ?
              0 : std::distance(lutIndex.begin(), std::prev(lower));
      upID = std::distance(lutIndex.begin(), upper);
    }
  } else { // lutIndex.back() < targetVal
    if (lower != lutIndex.end() &&
        eqvDouble(*lower, targetVal, prescision)) { // targetVal == *lower
      lowID = lutIndex.size() - 1;
      upID = lowID;
    } else { // lutIndex.back() < targetVal
      lowID = lutIndex.size() - 2;
      upID = lutIndex.size() - 1;
    }
  }
  return {lowID, upID};
}

static double getLut2Value(const library::LUT &lut,
                           double targetX, double targetY) {
  assert(lut.indexes.size() == 2);
  double res = NAN;
  const auto &indexX = lut.indexes[0];
  const auto &indexY = lut.indexes[1];
  auto [lowX, hiX] = getInterpolationIndexes(indexX, targetX);
  auto [lowY, hiY] = getInterpolationIndexes(indexY, targetY);
  if (lowX == hiX) {
    if (lowY == hiY) { // found exact value in table
      res = lut.getValue(lowX, lowY);
    } else { // linear interpolation on Y
      auto q1 = lut.getValue(lowX, lowY);
      auto q2 = lut.getValue(lowX, hiY);
      res = linearInterpolation(q1, q2, indexY[lowY], indexY[hiY], targetY);
    }
  } else {
    if (lowY == hiY) { // linear interpolation on X
      auto q1 = lut.getValue(lowX, lowY);
      auto q2 = lut.getValue(hiX, lowY);
      res = linearInterpolation(q1, q2, indexX[lowX], indexX[hiX], targetX);
    } else { // bilinear interpolation on X Y
      auto q11 = lut.getValue(lowX, lowY);
      auto q21 = lut.getValue(hiX, lowY);
      auto q12 = lut.getValue(lowX, hiY);
      auto q22 = lut.getValue(hiX, hiY);
      res = bilinearInterpolation(q11, q12, q21, q22,
                                  indexX[lowX], indexX[hiX], targetX,
                                  indexY[lowY], indexY[hiY], targetY);
    }
  }
  return res;
}

/// NOTE: if fanout is 0 resulting capacitance is also 0
double NLDM::cellOutputCapEstimation(const library::StandardCell &cell,
                                     size_t fanout){
  double capacitance = 0.0;
  std::for_each(cell.inputPins.begin(), cell.inputPins.end(),
    [&](const library::InputPin &pin) {
      capacitance += pin.capacitance;
    });
  if (!cell.inputPins.empty()) {
    capacitance = (capacitance / cell.inputPins.size()) * fanout;
  }
  return capacitance;
}

NLDM::EstimatedSD NLDM::cellOutputSDEstimation(
    const library::StandardCell &cell,
    double inputTransTime,
    double outputTotalCap) {
  NLDM::EstimatedSD estimatedSDC {0.0, 0.0};

  for (const auto &pin : cell.outputPins) {
    size_t timingArcNum = pin.delayFall.size();
    for (size_t i = 0; i < timingArcNum; ++i) {
      //TODO: properly handle LUTs with other sizes
      auto pinDelayFall = getLut2Value(pin.delayFall[i],
                                       inputTransTime, outputTotalCap);
      auto pinDelayRise = getLut2Value(pin.delayRise[i],
                                       inputTransTime, outputTotalCap);
      auto pinSlewFall = getLut2Value(pin.slewFall[i],
                                      inputTransTime, outputTotalCap);
      auto pinSlewRise = getLut2Value(pin.slewRise[i],
                                      inputTransTime, outputTotalCap);
      auto maxDelay = std::max(pinDelayFall, pinDelayRise);
      auto maxSlew = std::max(pinSlewFall, pinSlewRise);
      estimatedSDC.delay = std::max(estimatedSDC.delay, maxDelay);
      estimatedSDC.slew = std::max(estimatedSDC.slew, maxSlew);
    }
  }
  return estimatedSDC;
}

} // namespace eda::gate::estimator
