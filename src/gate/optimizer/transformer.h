//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace eda::gate::optimizer {

template <typename Builder>
using BuilderPtr = std::shared_ptr<Builder>;

/**
 * @brief Interface for component-to-component transformers.
 */
template <typename Builder>
class Transformer {
public:
  Transformer(const std::string &name): name(name) {}
  virtual ~Transformer() {}

  const std::string &getName() const { return name; }

  /// Processes the given component and contructs a new one. 
  virtual BuilderPtr<Builder> map(const BuilderPtr<Builder> &builder) const {
    return builder;
  }

private:
  const std::string name;
};

/**
 * @brief Interface for in-place component transformers.
 */
template <typename Builder>
class InPlaceTransformer : public Transformer<Builder> {
public:
  InPlaceTransformer(const std::string &name):
      Transformer<Builder>(name) {}
  virtual ~InPlaceTransformer() {}

  /// Transforms the component stored in the builder (in-place).
  virtual void transform(const BuilderPtr<Builder> &builder) const = 0;
};

/**
 * @brief Composite in-place component transformer.
 */
template <typename Builder>
class InPlaceTransformerChain final : public InPlaceTransformer<Builder> {
public:
  using Chain = std::vector<std::shared_ptr<InPlaceTransformer<Builder>>>;

  InPlaceTransformerChain(const std::string &name, const Chain &chain):
      InPlaceTransformer<Builder>(name), chain(chain) {}
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

  void transform(const BuilderPtr<Builder> &builder) const override {
    for (const auto &pass : chain) {
      pass->transform(builder);
    }
  }

private:
  const Chain chain;
};

} // namespace eda::gate::optimizer
