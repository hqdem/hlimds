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
  virtual std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) const = 0;

  /// Transforms the given subnet and returns the result of the transformation.
  SubnetID transform(const SubnetID subnetID) const {
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
  virtual void transform(SubnetBuilder &builder) const = 0;

  /// Default implementation of the base method.
  std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) const override {
    auto builder = std::make_unique<SubnetBuilder>(subnetID);
    transform(*builder);
    return builder;
  }
};

/// @brief Composite in-place subnet transformer.
class SubnetInPlaceTransformerChain final : public SubnetInPlaceTransformer {
public:
  using Chain = std::vector<std::shared_ptr<SubnetInPlaceTransformer>>;

  SubnetInPlaceTransformerChain(const Chain &chain): chain(chain) {}
  virtual ~SubnetInPlaceTransformerChain() {}

  void transform(SubnetBuilder &builder) const override {
    for (const auto &pass : chain) {
      pass->transform(builder);
    }
  }

private:
  const Chain chain;
};

} // namespace eda::gate::optimizer2
