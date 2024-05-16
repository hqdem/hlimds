//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <memory>
#include <sstream>
#include <string>

namespace eda::gate::optimizer {

/// @brief Interface for subnet-to-subnet transformers.
class SubnetTransformer {
public:
  using SubnetID = eda::gate::model::SubnetID;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  SubnetTransformer(const std::string &name): name(name) {}
  virtual ~SubnetTransformer() {}

  const std::string &getName() const { return name; }

  /// Transforms the given subnet and stores the result in the builder.
  virtual std::unique_ptr<SubnetBuilder> make(const SubnetID subnetID) const = 0;

  /// Transforms the given subnet and returns the result of the transformation.
  SubnetID transform(const SubnetID subnetID) const {
    auto builder = make(subnetID);
    return builder->make();
  }

private:
  const std::string name;
};

/// @brief Interface for in-place subnet transformers.
class SubnetInPlaceTransformer : public SubnetTransformer {
public:
  SubnetInPlaceTransformer(const std::string &name): SubnetTransformer(name) {}
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

  SubnetInPlaceTransformerChain(const std::string &name, const Chain &chain):
      SubnetInPlaceTransformer(name), chain(chain) {}
  virtual ~SubnetInPlaceTransformerChain() {}

  const Chain &getChain() const { return chain; }

  /// @brief Returns the string representation of the chain.
  const std::string getScript() const {
    std::stringstream ss;
    bool delimiter = false;
    for (const auto &pass : chain) {
      ss << (delimiter ? "; " : "") << pass->getName();
      delimiter = true;
    }
    return ss.str();
  }

  void transform(SubnetBuilder &builder) const override {
    for (const auto &pass : chain) {
      pass->transform(builder);
    }
  }

private:
  const Chain chain;
};

} // namespace eda::gate::optimizer
