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
 * @brief Maps technology-dependent designs to technology-independent ones.
 */
class DesignUnmapper final : public optimizer::DesignInPlaceTransformer {
public:
  using DesignBuilderPtr = std::shared_ptr<model::DesignBuilder>;

  DesignUnmapper(const std::string &name):
    optimizer::DesignInPlaceTransformer(name) {}

  void transform(const DesignBuilderPtr &builder) const override;
};

} // namespace eda::gate::techmapper
