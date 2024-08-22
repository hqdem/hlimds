//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"
#include "gate/optimizer/subnet_transformer.h"

namespace eda::gate::optimizer {

using DesignBuilderPtr = BuilderPtr<model::DesignBuilder>;

using DesignTransformer = Transformer<model::DesignBuilder>;

using DesignInPlaceTransformer = InPlaceTransformer<model::DesignBuilder>;

using DesignInPlaceTransformerChain =
  InPlaceTransformerChain<model::DesignBuilder>;

using DesignPass = std::shared_ptr<DesignInPlaceTransformer>;

using DesignMapper = std::shared_ptr<DesignTransformer>;

class EachSubnetInPlaceTransformer final : public DesignInPlaceTransformer {
public:
  EachSubnetInPlaceTransformer(const SubnetPass &pass):
      DesignInPlaceTransformer(pass->getName()), pass(pass) {}

  void transform(const DesignBuilderPtr &builder) const override {
    for (size_t i = 0; i < builder->getSubnetNum(); ++i) {
      pass->transform(builder->getSubnetBuilder(i));
    }
  }

private:
  const SubnetPass pass;
};

class EachSubnetTransformer final : public DesignInPlaceTransformer {
public:
  EachSubnetTransformer(const SubnetMapper &mapper):
      DesignInPlaceTransformer(mapper->getName()), mapper(mapper) {}

  void transform(const DesignBuilderPtr &builder) const override {
    for (size_t i = 0; i < builder->getSubnetNum(); ++i) {
      builder->setSubnetBuilder(i, mapper->map(builder->getSubnetBuilder(i)));
    }
  }

private:
  const SubnetMapper mapper;
};

} // namespace eda::gate::optimizer
