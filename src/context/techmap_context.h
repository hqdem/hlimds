//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"

#include <memory>

namespace eda::context {

struct TechMapContext final {
  std::unique_ptr<gate::library::SCLibrary> library;
};

} // namespace eda::context
