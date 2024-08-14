//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

#include <cassert>
#include <string>
#include <vector>

namespace eda::diag {

/**
 * @brief Diagnostics context.
 */
struct Context final {
  bool isEmpty() const {
    return scopes.empty();
  }

  size_t getDepth() const {
    return scopes.size();
  }

  void push(const std::string &scope) {
    scopes.push_back(scope);
  }

  void pop() {
    assert(!scopes.empty());
    scopes.resize(scopes.size() - 1);
  }

  std::vector<std::string> scopes;
};

/**
 * @brief Interface of diagnostics processors.
 */
struct Processor {
  Processor() {}
  virtual ~Processor() {}

  /// Begins the diagnostics output.
  virtual void onBegin(const Diagnostics &diagnostics) const = 0;
  /// Ends the diagnostics output.
  virtual void onEnd(const Diagnostics &diagnostics) const = 0;

  /// Begins the group of messages.
  virtual void onGroupBegin(
      const Entry &entry, const Context &context) const = 0;
  /// Ends the group of messages.
  virtual void onGroupEnd(
      const Entry &entry, const Context &context) const = 0;

  /// Processes the diagnostics entry (e.g., outputs to a terminal).
  virtual void onEntry(const Entry &entry, const Context &context) const = 0;

  void process(const Diagnostics &diagnostics) const;
};

} // namespace eda::diag
