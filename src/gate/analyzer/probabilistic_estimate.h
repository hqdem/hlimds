//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "bitset"

#include "gate/model/subnet.h"

#include "switching_activity.h"

namespace eda::gate::analyzer {

using Probabilities = std::vector<float>;
using Subnet        = eda::gate::model::Subnet;

/**
 * @brief Evaluates the switching activity by calculating the probability of switching  of each cell.
 */
class ProbabilisticEstimate : public SwitchActivityEstimator {

private:
 
  float combinations (size_t k, size_t n, std::vector<float> &prob) const;

  float majEstimate(std::vector<float> &majProb, size_t nMajP) const;

  float xorEstimate(std::vector<float> &xorProb, size_t nXorP) const;

public:

  /**
   * @brief Estimates the probability that the cell of Subnet will take the value 1.
   * 
   * @param subnet        The subnet for counting switches.
   * @param probabilities Probability distribution of inputs. Default value for each input is 0.5.
   * 
   * @return Vector of probability 1 for each cell of Subnet.
   */
  std::vector<float> probEstimator(const Subnet &subnet,
      const Probabilities &probabilities = {}) const;

  /**
   * @brief Estimates the switching activity by calculating the probability of switching  of each cell.
   * 
   * @param subnet        The subnet for counting switches.
   * @param probabilities Probability distribution of inputs. Default value for each input is 0.5.
   * 
   * @return The object of the parent class with total probability of switch from 1 to 0 and from 0 to 1.
   */
  SwitchActivity estimate(const Subnet &subnet,
      const Probabilities &probabilities = {}) const override;
};

} // namespace eda::gate::analyzer
