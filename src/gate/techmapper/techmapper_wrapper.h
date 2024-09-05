//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/criterion.h"
#include "gate/model/subnet.h"

namespace eda::gate::techmapper {

std::shared_ptr<model::SubnetBuilder> techMap(
  const criterion::Objective objective,
  const std::shared_ptr<model::SubnetBuilder> &builder
);

} // namespace eda::gate::techmapper
