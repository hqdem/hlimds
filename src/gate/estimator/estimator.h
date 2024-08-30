//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"
#include "gate/model/subnet.h"

#include <memory>

namespace eda::gate::estimator {

/**
 * @brief Interface for component estimators.
 */
template <typename Builder, typename Settings, typename Result>
class Estimator {
public:

  virtual void estimate(const std::shared_ptr<Builder> &builder,
                        const Settings &settings,
                        Result &result) const = 0;

  virtual ~Estimator() {}
};

//===----------------------------------------------------------------------===//
// Subnet Estimator
//===----------------------------------------------------------------------===//

template <typename Settings, typename Result>
using SubnetEstimator = Estimator<model::SubnetBuilder, Settings, Result>;

//===----------------------------------------------------------------------===//
// Design Estimator
//===----------------------------------------------------------------------===//

template <typename Settings, typename Result>
using DesignEstimator = Estimator<model::DesignBuilder, Settings, Result>;

template <typename Settings, typename Result>
class EachSubnetEstimator final : public DesignEstimator<Settings, Result> {
public:

  using EstimatorPtr = std::shared_ptr<SubnetEstimator<Settings, Result>>;

  EachSubnetEstimator(const EstimatorPtr &estimator): estimator(estimator) {}

  void estimate(const std::shared_ptr<model::DesignBuilder> &builder,
                const Settings &settings,
                Result &result) const override {

    for (size_t i = 0; i < builder->getSubnetNum(); ++i) {
      estimator->estimate(builder->getSubnetBuilder(i), settings, result);
    }
  }

private:
  const EstimatorPtr estimator;
};

} // namespace eda::gate::estimator

