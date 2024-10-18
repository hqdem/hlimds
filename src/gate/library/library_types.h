//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "kitty/dynamic_truth_table.hpp"
#include "util/double_math.h"
#include "util/npn_transformation.h"

#include <cmath> //for NAN
#include <map>
#include <optional>

namespace eda::gate::library {

  using namespace eda::util::doubleMath;
  struct WireLoadSelection {
      struct WireLoadFromArea {
        double leftBound = 0;
        double rightBound = 0;
        std::string wlmName = "";
      };
      std::vector<WireLoadFromArea> wlmFromArea;
    };

  struct InputPin;
  struct OutputPin;

  struct StandardCell {
    model::CellTypeID cellTypeID;
    std::vector<kitty::dynamic_truth_table> ctt; // canonized TT
    std::vector<util::NpnTransformation> transform;
    //TODO:  what type of nan to use?
    double propertyArea = NAN;
    double propertyDelay = NAN;
    double propertyLeakagePower = NAN;
    std::vector<InputPin> inputPins;
    std::vector<OutputPin> outputPins;

    std::string name;
  };

  struct WireLoadModel {
    struct FanoutLength {
      size_t fanoutCount = 0;
      double length = 0;

      friend bool operator==(const FanoutLength &lhs,
                             const FanoutLength &rhs) {
        return (lhs.fanoutCount == rhs.fanoutCount)
                && eqvDouble(lhs.length, rhs.length);
      }
    };

    WireLoadModel() = default;
    ~WireLoadModel() = default;
    WireLoadModel(const std::string &name, double resistance, double capacitance,
                  double slope, const std::vector<FanoutLength> &fanoutLength):
      name(name), resistance(resistance), capacitance(capacitance),
      slope(slope), wireLength(fanoutLength) {};

    friend bool operator==(const WireLoadModel &lhs,
                           const WireLoadModel &rhs) {
      return (lhs.name == rhs.name)
             && eqvDouble(lhs.resistance, rhs.resistance)
             && eqvDouble(lhs.capacitance, rhs.capacitance)
             && eqvDouble(lhs.slope, rhs.slope)
             && (lhs.wireLength == rhs.wireLength);
    };

    friend bool operator!=(const WireLoadModel &lhs,
                           const WireLoadModel &rhs) {
      return !(lhs == rhs);
    };

    double getFanoutLength(size_t fanoutCount) const {
      assert(fanoutCount > 0);
      if (fanoutCount > wireLength.size()) {
        return slope * (fanoutCount - wireLength.size()) *
               wireLength.back().length;
      }
      return wireLength[fanoutCount - 1].length;
    };

    double getFanoutCapacitance(size_t fanoutCount) const {
      return getFanoutLength(fanoutCount) * capacitance;
    };

    std::string name;
    double resistance = 0.0;
    double capacitance = 0.0;
    double slope = 0.0;
    std::vector<FanoutLength> wireLength;
  };

  struct LutTemplate {
    //In each template might be from 1 to 4 variables
    //for each variable there are several values.

    enum class NameID {
      UndefinedVariable = -1,
      InputNetTransition,
      InputNoiseHeight,
      InputNoiseWidth,
      InputVoltage,
      OutputNetLength,
      OutputNetWireCap,
      OutputNetPinCap,
      OutputVoltage,
      OutputTransition,
      OutputPinTransition,
      RelatedOutTotalOutputNetCapacitance,
      RelatedOutOutputNetLength,
      RelatedOutOutputNetWireCap,
      RelatedOutOutputNetPinCap,
      RelatedPinTransition,
      FanoutNumber,
      FanoutPinCapacitance,
      TotalOutputNetCapacitance,
      NormalizedVoltage,
      Time,
      ConstrainedPinTransition,
      DriverSlew,
      RcProduct,
      ConnectDelay,
      Template_End,
      CurveParameters,
      OutputNetTransition,
      InputTransitionTime,
      Frequency,
      EqualOrOppositeNetCapacitance,
      DefectSizeDiameter
    };

    std::string name;
    std::vector<NameID> variables;
    std::vector<std::vector<double>> indexes;
  };

  struct LUT {
    LUT() = default;
    ~LUT() = default;

    LUT(const LUT &other) = default;
    LUT& operator=(const LUT& other) = default;
    LUT(LUT &&other) = default;
    LUT& operator=(LUT&& other) = default;

    std::string name;
    std::vector<std::vector<double>> indexes;
    std::vector<double> values;
    double getValue(size_t i, size_t j) const {
      return values[i * indexes[0].size() + j];
    }
  };

  struct Pin {
    std::string name;
    std::vector<LUT> powerFall, powerRise;
  };

  struct InputPin : Pin {
    double capacitance = 0.0;
    double fallCapacitance = 0.0;
    double riseCapacitance = 0.0;
  };

  struct OutputPin : Pin {
    //TODO: according to Liberty might have "capacitance" atribute
    //but in practise onlu input pins have it
    double maxCapacitance = 0.0;
    std::vector<LUT> delayFall, delayRise;
    std::vector<LUT> slewFall, slewRise;

    std::vector<int> timingSence; //TODO: probably used incorrectly
    std::string stringFunction; //TODO: probably should not be like that
  };

} // namespace eda::gate::library