//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "context/techmap_context.h"
#include "gate/criterion/criterion.h"

#include <memory>

namespace eda::context {

struct UtopiaContext final {
  std::unique_ptr<gate::criterion::Criterion> criterion;

  TechMapContext techMapContext;
};

} //namespace eda::context
