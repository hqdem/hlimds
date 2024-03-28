//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <memory>

namespace eda::gate::optimizer2 {

/// @brief Interface for subnet-to-subnet transformers.
class SubnetTransformer {
public:
  using SubnetID = eda::gate::model::SubnetID;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  SubnetTransformer() {}
  virtual ~SubnetTransformer() {}

  /// Transforms the given subnet and stores the result in the builder.
  virtual std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) = 0;

  /// Transforms the given subnet and returns the result of the transformation.
  SubnetID transform(const SubnetID subnetID) {
    auto builder = make(subnetID);
    return builder->make();
  }
};

/// @brief Interface for in-place subnet transformers.
class SubnetInPlaceTransformer : public SubnetTransformer {
public:
  SubnetInPlaceTransformer() {}
  virtual ~SubnetInPlaceTransformer() {}

  /// Transforms the subnet stored in the builder (in-place).
  virtual void transform(SubnetBuilder &builder) = 0;

  /// Default implementation of the base method.
  std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) override {
    auto builder = std::make_unique<SubnetBuilder>(subnetID);
    transform(*builder);
    return builder;
  }
};

} // namespace eda::gate::optimizer2
