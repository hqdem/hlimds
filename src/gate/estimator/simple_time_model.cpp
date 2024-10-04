//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simple_time_model.h"
#include "util/env.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::estimator {

/// Interpolation
double NLDM::getLutValue(const std::vector<double> &lutValues,
                        DataTimingContext &context,
                        const double x1, const double x2,
                        const double y1, const double y2) {
  if (!context.interpolate) {
    return lutValues[context.index.ind1 * context.variablesCount + context.index.ind2];
  }

  /// Properties
  double x0 = context.inputTransTime;
  double y0 = context.outputTotalCap;
  double x01 = 0, x20 = 0, y01 = 0, y20 = 0;
  double T00 = 0, T11 = 0, T12 = 0, T21 = 0, T22 = 0;

  /// Estimation
  x01 = (x0 - x1) / (x2 - x1);
  x20 = (x2 - x0) / (x2 - x1);
  y01 = (y0 - y1) / (y2 - y1);
  y20 = (y2 - y0) / (y2 - y1);

  T11 = lutValues[context.index.back1 * context.variablesCount + context.index.back2];
  T12 = lutValues[context.index.back1 * context.variablesCount + context.index.front2];
  T21 = lutValues[context.index.front1 * context.variablesCount + context.index.back2];
  T22 = lutValues[context.index.front1 * context.variablesCount + context.index.front2];

  /// Result value
  T00 = x20*y20*T11 + x20*y01*T12 + x01*y20*T21 + x01*y01*T22;
  return T00;
}

void NLDM::pinTimingEstimator(//const std::vector<const LookupTable*> &luts,
                              const std::vector<const library::LUT*> &luts,
                              DataTimingContext &context) {

  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<double> lutValues;
  /// Values for interpolation
  double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

  //===-----------------------------------------------------------------===//
  //  Assign values from LookUp Tables
  //===-----------------------------------------------------------------===//
  if (luts[0] == nullptr) {
    std::cerr << "Cell fall LUT is nullptr.\n";
    assert(false);
    return;
  }
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  lutValues = luts[0]->values;
  bool iterFlag = false;
  //===-----------------------------------------------------------------===//
  for (const auto &lutIndex : luts[0]->indexes) {
    /// INTERPOLATION
    if (std::find(lutIndex.begin(), lutIndex.end(),
          context.inputTransTime) == std::end(lutIndex)) {
      if (!iterFlag) {
        /// Assign net_transition value
        for (size_t i = 0; i < lutIndex.size(); ++i) {
          if (lutIndex[i] < context.inputTransTime) {
            context.index.back1 = i;
            x1 = lutIndex[i];
          }
          else if (lutIndex[i] > context.inputTransTime) {
            context.index.front1 = i;
            x2 = lutIndex[i];
            context.variablesCount = lutIndex.size();
            iterFlag = true;
            break;
          }
        }
      }
      else {
        /// Assign output capacitance value
        for (size_t i = 0; i < lutIndex.size(); ++i) {
          if (lutIndex[i] < context.outputTotalCap) {
            context.index.back2 = i;
            y1 = lutIndex[i];
          }
          else if (lutIndex[i] > context.outputTotalCap) {
            context.index.front2 = i;
            y2 = lutIndex[i];
            break;
          }
        }
        bool tmpInterpolate = context.interpolate; // TODO
        context.interpolate = true;
        context.delayValues.push_back(
              getLutValue(lutValues, context, x1, x2, y1, y2));
        context.interpolate = tmpInterpolate;
        break;
      }
    }

    /// Assign found values
    else if (std::find(lutIndex.begin(), lutIndex.end(),
                          context.inputTransTime) != std::end(lutIndex)) {
      if (!iterFlag) {
        /// Assign net_transition value
        for (size_t i = 0; i < lutIndex.size(); ++i) {
          if (lutIndex[i] == context.inputTransTime) {
            context.index.ind1 = i;
            iterFlag = true;
            break;
          }
        }
      }
      else {
        /// Assign output capacitance value
        for (size_t i = 0; i < lutIndex.size(); ++i) {
          if (lutIndex[i] == context.outputTotalCap) {
            context.index.ind2 = i;
            context.variablesCount = lutIndex.size();
            break;
          }
        }
        context.delayValues.push_back(
          lutValues[context.index.ind1 * context.variablesCount + context.index.ind2]);
        break;
      }
    }
  }
  

  context.interpolate = (context.index.ind1 == -1) && (context.index.ind2 == -1);

  for (size_t i = 1; i < 4; ++i) {
    if (luts[i] == nullptr) {
      std::cerr << "LUT is nullptr.\n";
      assert(false);
      return;
    }
    if (luts[i]->values.size() != 1) {
      context.delayValues.push_back(
        getLutValue(luts[i]->values, context, x1, x2, y1, y2));
    } else {
      context.delayValues.push_back(0);
    }
  }
}

void NLDM::pinFTimingEstimator(//const std::vector<const LookupTable*> &luts,
                               const std::vector<const library::LUT*> &luts,
                               DataTimingContext &context) {
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<double> lutValues, result;
  //===-----------------------------------------------------------------===//
  for (size_t i = 0; i < 4; ++i) {
    if (luts[i] != nullptr) {
      lutValues = luts[i]->values;
      result.push_back(
        lutValues[context.index.ind1 * context.variablesCount + context.index.ind2]);
    }
    else {
      /// Found cell has a "scalar" LUT
      result.push_back(0);
    }
  }

  context.delayValues = result;
}

void NLDM::pinITimingEstimator(//const std::vector<const LookupTable*> &luts,
                               const std::vector<const library::LUT*> &luts,
                               DataTimingContext &context) {
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<double> lutValues, result;
  double x1 = 0, x2 = 0,
         y1 = 0, y2 = 0;
  bool iterFlag = false;

  assert(luts[0] != nullptr);
  for (const auto &lutIndex : luts[0]->indexes) {
  //for (const auto &it : (*luts[0])) {
    if (!iterFlag) {
      x1 = lutIndex[context.index.back1];
      x2 = lutIndex[context.index.front1];
      iterFlag = true;
    }
    else {
      y1 = lutIndex[context.index.back2];
      y2 = lutIndex[context.index.front2];
    }
  }
  //===-----------------------------------------------------------------===//
  bool tmpInterpolation = context.interpolate; // TODO
  context.interpolate = true;
  for (size_t i = 0; i < 4; ++i) {
    if (luts[i] != nullptr) {
      lutValues = luts[i]->values;
      result.push_back(getLutValue(lutValues, context, x1, x2, y1, y2));
    }
    else {
      /// Found cell has a "scalar" LUT
      result.push_back(0);
    }
  }
  context.interpolate = tmpInterpolation;
  context.delayValues = result;
}

void NLDM::delayEstimation(const library::StandardCell &cell,
                           const double inputTransTime,
                           const double outputTotalCap,
                           int &timingSense, double &slew, double &delay, double &cap) {
  //===---------------------------------------------------------------===//
  //  Properties
  //===---------------------------------------------------------------===//
  std::vector<double> cfall, crise, tfall, trise;
  bool readyFlag = false;
  int newTimingSense = timingSense;
  DataTimingContext context{inputTransTime, outputTotalCap};

  //===---------------------------------------------------------------===//
  //  Delay and Slew estimation
  //===---------------------------------------------------------------===//
  cap = 0;
  std::for_each(cell.inputPins.begin(), cell.inputPins.end(),
    [&](const library::InputPin &pin) {
      cap += pin.capacitance;
    });
  if (!cell.inputPins.empty()) {
    cap = cap / cell.inputPins.size();
  }
  for (const auto &pin : cell.outputPins) {
    for (size_t i = 0; i < pin.delayFall.size(); ++i) {
      std::vector<const library::LUT*> luts {
        &pin.delayFall[i],
        &pin.delayRise[i],
        &pin.slewFall[i],
        &pin.slewRise[i]};

      if (!readyFlag) {
        context.interpolate = true;
        pinTimingEstimator(luts, context);
        readyFlag = true;
      } else {
        if (!context.interpolate) {
          pinFTimingEstimator(luts, context);
        }
        else {
          pinITimingEstimator(luts, context);
        }
      }
      cfall.push_back(context.delayValues[0]);
      crise.push_back(context.delayValues[1]);
      tfall.push_back(context.delayValues[2]);
      trise.push_back(context.delayValues[3]);
      newTimingSense = pin.timingSence[i];
    }
  }

  if (crise.empty() || cfall.empty() || trise.empty() || tfall.empty()) {
    delay = 0;
    slew = 0;
  } else {
    if (timingSense == 0) {
      delay = *max_element(crise.begin(), crise.end());
      for (size_t i = 0; i < crise.size(); ++i)
        if (crise[i] == delay) {
          slew = (tfall[i] + trise[i])/2;
          break;
        }
    }
    else {
      delay = *max_element(cfall.begin(), cfall.end());
      for (size_t i = 0; i < cfall.size(); ++i)
        if (cfall[i] == delay) {
          slew = (tfall[i] + trise[i])/2;
          break;
        }
    }
  }
  timingSense = newTimingSense;
}

double NLDM::delayEstimation(const library::StandardCell &cell,
                            const double inputTransTime,
                            const double outputTotalCap) {
  int timingSense = 0;
  double slew = 0;
  double delay = 0;
  double cap = 0;
  delayEstimation(cell, inputTransTime, outputTotalCap,
    timingSense, slew, delay, cap);
  return slew;
}

} // namespace eda::gate::estimator
