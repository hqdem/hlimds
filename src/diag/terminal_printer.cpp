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

static constexpr auto NoteColor  = fmt::terminal_color::bright_green;
static constexpr auto WarnColor  = fmt::terminal_color::bright_yellow;
static constexpr auto ErrorColor = fmt::terminal_color::bright_red;
static constexpr auto OtherColor = fmt::terminal_color::white;

static inline fmt::terminal_color getSeverityColor(const uint8_t lvl) {
  switch (lvl) {
  case NOTE:  return NoteColor;
  case WARN:  return WarnColor;
  case ERROR: return ErrorColor;
  default:    return OtherColor;
  }
}

static inline void printErrorNum(const unsigned n) {
  fmt::print(fmt::emphasis::bold | fg(ErrorColor), "{} errors", n);
}

static inline void printWarnNum(const unsigned n) {
  fmt::print(fmt::emphasis::bold | fg(WarnColor), "{} warnings", n);
}

void TerminalPrinter::onBegin(const Diagnostics &diagnostics) const {
  // Do nothing.
}

void TerminalPrinter::onEntry(const Entry &entry, const unsigned depth) const {
  static constexpr auto indent = 2 /* space(s) */;

  const auto lvl = entry.lvl;
  fmt::print(std::string(indent * depth, ' '));
  if (lvl != GROUP) {
    fmt::print(fg(getSeverityColor(lvl)), "{}: ", getSeverityString(lvl));
  }
  fmt::print(entry.msg);
  fmt::print("\n");
}

void TerminalPrinter::onEnd(const Diagnostics &diagnostics) const {
  const auto nError = diagnostics.getErrorNum();
  const auto nWarn = diagnostics.getWarnNum();

  const auto needsSummary = (nError > 0) || (nWarn > 0);

  if (needsSummary) {
    fmt::print(fmt::emphasis::bold, "execution finished with ");
  }

  if ((nError > 0) && (nWarn > 0)) {
    printErrorNum(nError);
    fmt::print(" and ");
    printWarnNum(nWarn);
  } else if (nError > 0) {
    printErrorNum(nError);
  } else if (nWarn > 0) {
    printWarnNum(nError);
  }

  if (needsSummary) {
    fmt::print("\n");
  }
}

} // namespace eda::diag
