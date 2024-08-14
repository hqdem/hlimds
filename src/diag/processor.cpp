//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/processor.h"

#include <stack>

namespace eda::diag {

using DfsStack = std::stack<std::pair<const Entry *, size_t>>;

static inline void push(DfsStack &stack, Context &context, const Entry *entry) {
  stack.push({entry, 0});
  context.push(entry->msg);
}

static inline void pop(DfsStack &stack, Context &context) {
  stack.pop();
  if (!context.isEmpty())
    context.pop();
}

void Processor::process(const Diagnostics &diagnostics) const {
  onBegin(diagnostics);

  DfsStack stack;
  // The root entry is not processed.
  stack.push({&diagnostics.getDiagnosis(), 0});

  Context context{};
  while (!stack.empty()) {
    auto &[entry, index] = stack.top();

    if (index < entry->more.size()) {
      const auto *subentry = &entry->more[index];

      if (!subentry->more.empty())
        onGroupBegin(*subentry, context);
      else
        onEntry(*subentry, context);

      push(stack, context, subentry);
      index++;
    } else {
      pop(stack, context);

      if (!entry->more.empty())
        onGroupEnd(*entry, context);
    }
  } // while DFS

  onEnd(diagnostics);
}

} // namespace eda::diag
