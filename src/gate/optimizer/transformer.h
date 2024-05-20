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

//===----------------------------------------------------------------------===//
// Base templates
//===----------------------------------------------------------------------===//

/// @brief Interface for component-to-component transformers.
template <typename ID, typename Builder>
class Transformer {
public:
  Transformer(const std::string &name): name(name) {}
  virtual ~Transformer() {}

  const std::string &getName() const { return name; }

  /// Transforms the given component and stores the result in the builder.
  virtual std::unique_ptr<Builder> make(const ID componentID) const = 0;

  /// Transforms the given component and returns the result of the transformation.
  ID transform(const ID componentID) const {
    auto builder = make(componentID);
    return builder->make();
  }

private:
  const std::string name;
};

/// @brief Interface for in-place component transformers.
template <typename ID, typename Builder>
class InPlaceTransformer : public Transformer<ID, Builder> {
public:
  InPlaceTransformer(const std::string &name):
      Transformer<ID, Builder>(name) {}
  virtual ~InPlaceTransformer() {}

  /// Transforms the component stored in the builder (in-place).
  virtual void transform(Builder &builder) const = 0;

  /// Default implementation of the base method.
  std::unique_ptr<Builder> make(const ID componentID) const override {
    auto builder = std::make_unique<Builder>(componentID);
    transform(*builder);
    return builder;
  }
};

/// @brief Composite in-place component transformer.
template <typename ID, typename Builder>
class InPlaceTransformerChain final : public InPlaceTransformer<ID, Builder> {
public:
  using Chain = std::vector<std::shared_ptr<InPlaceTransformer<ID, Builder>>>;

  InPlaceTransformerChain(const std::string &name, const Chain &chain):
      InPlaceTransformer<ID, Builder>(name), chain(chain) {}
  virtual ~InPlaceTransformerChain() {}

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

  void transform(Builder &builder) const override {
    for (const auto &pass : chain) {
      pass->transform(builder);
    }
  }

private:
  const Chain chain;
};

//===----------------------------------------------------------------------===//
// Subnet transformers
//===----------------------------------------------------------------------===//

using SubnetTransformer =
    Transformer<model::SubnetID, model::SubnetBuilder>;

using SubnetInPlaceTransformer =
    InPlaceTransformer<model::SubnetID, model::SubnetBuilder>;

using SubnetInPlaceTransformerChain =
    InPlaceTransformerChain<model::SubnetID, model::SubnetBuilder>;

} // namespace eda::gate::optimizer
