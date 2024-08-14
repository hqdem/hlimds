//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/processor.h"

namespace eda::diag {

struct TerminalPrinter final : public Processor {
  static constexpr auto NoHierarchy = true;
  static constexpr auto IndentWidth = 0;

  void onBegin(const Diagnostics &diagnostics) const override;
  void onEnd(const Diagnostics &diagnostics) const override;

  void onGroupBegin(
      const Entry &entry, const Context &context) const override;
  void onGroupEnd(
      const Entry &entry, const Context &context) const override;

  void onEntry(const Entry &entry, const Context &context) const override;
};

} // namespace eda::diag
