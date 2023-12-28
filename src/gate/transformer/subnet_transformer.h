//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

namespace eda::gate::transformer {

/// @brief Common interface for subnet transformers.
class SubnetTransformer {
public:
  using SubnetID = eda::gate::model::SubnetID;

  SubnetTransformer() {}
  virtual ~SubnetTransformer() {}

  virtual SubnetID transform(SubnetID id) = 0;
};

} // namespace eda::gate::transformer
