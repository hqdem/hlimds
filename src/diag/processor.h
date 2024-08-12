//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

namespace eda::diag {

class Processor {
public:
  Processor() {}
  virtual ~Processor() {}

  /// Process the diagnostics entry (e.g., outputs to terminal).
  virtual void onEntry(const Entry &entry) const = 0;

  void process(const Diagnostics &diagnostics) const {
    const auto &entries = diagnostics.get();
    for (const auto &entry : entries) {
      onEntry(entry);
    }
  }
};

} // namespace eda::diag
