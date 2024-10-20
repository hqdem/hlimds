//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"
#include "gate/model/subnet.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace eda::gate::optimizer {

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
  virtual std::shared_ptr<Builder> map(
      const std::shared_ptr<Builder> &builder) const {
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
  virtual void transform(const std::shared_ptr<Builder> &builder) const = 0;
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

  void transform(const std::shared_ptr<Builder> &builder) const override {
    for (const auto &pass : chain) {
      pass->transform(builder);
    }
  }

private:
  const Chain chain;
};

//===----------------------------------------------------------------------===//
// Subnet Transformer
//===----------------------------------------------------------------------===//

using SubnetTransformer =
    Transformer<model::SubnetBuilder>;

using SubnetInPlaceTransformer =
    InPlaceTransformer<model::SubnetBuilder>;

using SubnetInPlaceTransformerChain =
    InPlaceTransformerChain<model::SubnetBuilder>;

using SubnetPass = std::shared_ptr<SubnetInPlaceTransformer>;
using SubnetMapper = std::shared_ptr<SubnetTransformer>;

//===----------------------------------------------------------------------===//
// Design Transformer
//===----------------------------------------------------------------------===//

using DesignTransformer =
    Transformer<model::DesignBuilder>;

using DesignInPlaceTransformer =
    InPlaceTransformer<model::DesignBuilder>;

using DesignInPlaceTransformerChain =
    InPlaceTransformerChain<model::DesignBuilder>;

using DesignPass = std::shared_ptr<DesignInPlaceTransformer>;
using DesignMapper = std::shared_ptr<DesignTransformer>;

class EachSubnetInPlaceTransformer final : public DesignInPlaceTransformer {
public:
  EachSubnetInPlaceTransformer(const SubnetPass &pass,
                               const bool skipTrivial = true):
      DesignInPlaceTransformer(pass->getName()),
      pass(pass),
      skipTrivial(skipTrivial) {}

  void transform(
      const std::shared_ptr<model::DesignBuilder> &builder) const override {
    for (size_t i = 0; i < builder->getSubnetNum(); ++i) {
      const auto &subnetBuilder = builder->getSubnetBuilder(i);
      if (!(skipTrivial && subnetBuilder->isTrivial())) {
        pass->transform(builder->getSubnetBuilder(i));
      }
    }
  }

private:
  const SubnetPass pass;
  const bool skipTrivial;
};

class EachSubnetTransformer final : public DesignInPlaceTransformer {
public:
  EachSubnetTransformer(const SubnetMapper &mapper,
                        const bool skipTrivial = true):
      DesignInPlaceTransformer(mapper->getName()),
      mapper(mapper),
      skipTrivial(skipTrivial) {}

  void transform(
      const std::shared_ptr<model::DesignBuilder> &builder) const override {
    for (size_t i = 0; i < builder->getSubnetNum(); ++i) {
      const auto &subnetBuilder = builder->getSubnetBuilder(i);
      if (!(skipTrivial && subnetBuilder->isTrivial())) {
        builder->setSubnetBuilder(i, mapper->map(subnetBuilder));
      }
    }
  }

private:
  const SubnetMapper mapper;
  const bool skipTrivial;
};

} // namespace eda::gate::optimizer
