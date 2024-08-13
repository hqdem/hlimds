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

class TerminalPrinter final : public Processor {
public:
  virtual void onBegin(const Diagnostics &diagnostics) const override;
  virtual void onEntry(const Entry &entry, const unsigned depth) const override;
  virtual void onEnd(const Diagnostics &diagnostics) const override;
};

} // namespace eda::diag
