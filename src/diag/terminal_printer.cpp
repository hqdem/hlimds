//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/terminal_printer.h"

#include <iostream>

namespace eda::diag {

void TerminalPrinter::onEntry(const Entry &entry) const {
  std::cerr << entry.getLevel() << ": " << entry.msg << std::endl;
}

} // namespace eda::diag
