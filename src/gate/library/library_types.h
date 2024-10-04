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
#include "util/npn_transformation.h"

#include <cmath> //for NAN
#include <map>
#include <optional>

namespace eda::gate::library {
 
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
    };
    std::string name;
    /// @brief resistance, capacitance, extrapolation slope
    double resistance = 0.0;
    double capacitance = 0.0;
    double slope = 0.0;
    std::vector<FanoutLength> wireLength;
    
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
  };

  struct LutTemplate {
    //In each template might be from 1 to 4 variables
    //for each variable there are several values.
    struct Variable {
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

      NameID nameID;
      std::vector<double> values;
    };
    std::vector<Variable> variables;
  };
  
  struct LUT {
    LUT() = default;
    ~LUT() = default;
    
    LUT(const LUT &other) = default;
    LUT& operator=(const LUT& other) = default;
    LUT(LUT &&other) = default;
    LUT& operator=(LUT&& other) = default;

    std::vector<std::vector<double>> indexes; //TODO: should it be here?
    std::vector<double> values;
  };

  struct Pin {
    std::string name;
    double capacitance = 0.0;
    std::vector<LUT> LUTs;
  };

  struct InputPin : Pin {
    double fallCapacitance = 0.0, 
          riseCapacitance = 0.0;

    //TODO: might need own realization of std::optional to save memory
    std::optional<LUT> powerFall;
    std::optional<LUT> powerRise;
  };

  struct OutputPin : Pin {
    //the maximum total capacitive load that an output pin can drive.
    double maxCapacitance = 0.0;
    std::vector<LUT> delayFall, delayRise;
    std::vector<LUT> slewFall, slewRise;

    std::vector<LUT> powerFall, powerRise;

    std::vector<int> timingSence; //TODO: probably used incorrectly
    std::string stringFunction; //TODO: probably should not be like that
  };

} // namespace eda::gate::library