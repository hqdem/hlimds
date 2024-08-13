//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/terminal_printer.h"

#include <fmt/color.h>
#include <fmt/format.h>

#include <iostream>

namespace eda::diag {

static inline fmt::terminal_color getSeverityColor(const uint8_t lvl) {
  switch (lvl) {
  case NOTE:  return fmt::terminal_color::bright_green;
  case WARN:  return fmt::terminal_color::bright_yellow;
  case ERROR: return fmt::terminal_color::bright_red;
  default:    return fmt::terminal_color::white;
  }
}

void TerminalPrinter::onEntry(const Entry &entry, const unsigned depth) const {
  static constexpr auto indent = 2;

  fmt::print(std::string(indent * depth, ' '));
  if (entry.lvl != GROUP) {
    fmt::print(fg(getSeverityColor(entry.lvl)), getSeverityString(entry.lvl));
    fmt::print(": ");
  }
  fmt::print(entry.msg);
  fmt::print("\n");
}

} // namespace eda::diag
