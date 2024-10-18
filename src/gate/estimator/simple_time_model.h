//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library_types.h"

#include <string>
#include <vector>

namespace eda::gate::estimator {

//===---------------------------------------------------------------------===//
// NLDM
//===---------------------------------------------------------------------===//

/*
 *  Non-Linear Delay Model
 *  Basic class for delay estimation.
 */
class NLDM {
public:
  virtual ~NLDM() = default;

  //represents estimated values of Slew and Delay for cell as a whole
  struct EstimatedSD {
    double slew = NAN;
    double delay = NAN;
  };

  /// @brief
  /// @param cell from SCLibrary
  /// @param fanout amount of output links
  /// @return
  static double cellOutputCapEstimation(const library::StandardCell &cell,
                                        size_t fanout);
  /// @brief Rough estimation of cell Slew and Delay.
  /// @param cell from SCLibrary
  /// @param inputTransTime single delay value that is assumed for each input pin.
  /// @param outputTotalCap single output capacitance that is assumed for each output pin.
  /// @return Slew and Delay calculated as maximum values among rise and fall
  /// of each timing arc.
  static EstimatedSD cellOutputSDEstimation(
    const library::StandardCell &cell,
    double inputTransTime,
    double outputTotalCap);
};



} // namespace eda::gate::estimator
