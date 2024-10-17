//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
 
#pragma once

#include "gate/optimizer/transformer.h"

namespace eda::gate::techmapper {

/**
 * @brief Maps technology-dependent subnets to technology-independent ones.
 */
class SubnetUnmapper final : public optimizer::SubnetTransformer {
public:
  using SubnetBuilderPtr = std::shared_ptr<model::SubnetBuilder>;

  SubnetUnmapper(const std::string &name):
    optimizer::SubnetTransformer(name) {}

  SubnetBuilderPtr map(const SubnetBuilderPtr &builder) const override;
};

} // namespace eda::gate::techmapper
