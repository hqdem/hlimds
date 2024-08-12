//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/diagnostics.h"

namespace eda::diag {

std::string Entry::getLevel() const {
  switch (lvl) {
  case NOTE:
    return "note";
  case WARN:
    return "warning";
  case ERROR:
    return "error";
  default:
    return "unknown";
  }
}

} // namespace eda::diag
