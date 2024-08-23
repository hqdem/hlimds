//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "cut.h"

namespace eda::gate::optimizer {

bool Cut::dominates(const Cut &other) const {
  if (entryIdxs.size() >= other.entryIdxs.size()) {
    return false;
  }
  for (const auto &entryIdx : entryIdxs) {
    if (other.entryIdxs.find(entryIdx) == other.entryIdxs.end()) {
      return false;
    }
  }
  return true;
}

} // namespace eda::gate::optimizer
