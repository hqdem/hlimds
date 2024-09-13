//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/switching_activity.h"
#include "gate/model/subnet.h"
#include "util/singleton.h"

#include <bitset>

namespace eda::gate::estimator {

/**
 * @brief Evaluates the switching activity
 * by calculating the probability of switching of each cell.
 */
class ProbabilityEstimator : public SwitchActivityEstimator {
public:
  /// @cond ALIASES
  using Builder       = eda::gate::model::SubnetBuilder;
  using Cell          = eda::gate::model::Subnet::Cell;
  using LinkList      = eda::gate::model::Subnet::LinkList;
  using Probabilities = SwitchActivity::Probabilities;
  using Subnet        = eda::gate::model::Subnet;
  /// @endcond

private:
 
  float combinations (size_t k, size_t n, std::vector<float> &prob) const;

  float majEstimate(std::vector<float> &majProb, size_t nMajP) const;

  float xorEstimate(std::vector<float> &xorProb, size_t nXorP) const;

  void estimateCell(const Builder &builder, Probabilities &probs,
                    const size_t i, const Probabilities &inProbs) const;

public:

  ProbabilityEstimator() {};

  /**
   * @brief Estimates the probability
   * that the cell of Subnetbuilder will take the value 1.
   * 
   * @param builder       The SubnetBuilder for counting switches.
   * @param probabilities Probability distribution of inputs. Default is 0.5.
   * 
   * @return Vector of probability 1 for each cell of Subnet.
   */
  Probabilities estimateProbs(const Builder &builder,
                              const Probabilities &probabilities = {}) const;
  /**
   * @brief Estimates the switching activity
   * by calculating the probability of switching of each cell.
   * 
   * @param builder       The subnet builder for counting switches.
   * @param probabilities Probability distribution of inputs. Default is 0.5.
   * @param result        The total probabilities of switching.
   */
  void estimate(const std::shared_ptr<Builder> &builder,
                const Probabilities &probabilities,
                SwitchActivity &result) const override;
};

} // namespace eda::gate::estimator
