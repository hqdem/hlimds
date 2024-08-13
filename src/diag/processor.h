//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

#include <stack>

namespace eda::diag {

class Processor {
public:
  Processor() {}
  virtual ~Processor() {}

  /// Begins the diagnostics output.
  virtual void onBegin(const Diagnostics &diagnostics) const = 0;

  /// Process the diagnostics entry (e.g., outputs to a terminal).
  virtual void onEntry(const Entry &entry, const unsigned depth) const = 0;

  /// Ends the diagnostics output.
  virtual void onEnd(const Diagnostics &diagnostics) const = 0;

  void process(const Diagnostics &diagnostics) const {
    onBegin(diagnostics);

    std::stack<std::pair<const Entry *, size_t>> dfsStack;
    // The root entry is not processed.
    dfsStack.push({&diagnostics.getDiagnosis(), 0});

    while (!dfsStack.empty()) {
      auto &[entry, index] = dfsStack.top();

      if (index < entry->more.size()) {
        const auto *subentry = &entry->more[index];
        onEntry(*subentry, dfsStack.size() - 1);
        dfsStack.push({subentry, 0});
        index++;
      } else {
        dfsStack.pop();
      }
    } // while DFS

    onEnd(diagnostics);
  }
};

} // namespace eda::diag
