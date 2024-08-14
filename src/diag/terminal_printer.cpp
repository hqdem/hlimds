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
static constexpr auto GroupColor = fmt::terminal_color::bright_black;
static constexpr auto OtherColor = fmt::terminal_color::white;

static inline fmt::terminal_color getSeverityColor(const uint8_t lvl) {
  switch (lvl) {
  case NOTE:  return NoteColor;
  case WARN:  return WarnColor;
  case ERROR: return ErrorColor;
  default:    return OtherColor;
  }
}

static inline void printIndent(const unsigned n) {
  fmt::print(std::string(TerminalPrinter::IndentWidth * n, ' '));
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

void TerminalPrinter::onEnd(const Diagnostics &diagnostics) const {
  const auto nError = diagnostics.getErrorNum();
  const auto nWarn = diagnostics.getWarnNum();

  const auto needsSummary = (nError > 0) || (nWarn > 0);

  if (needsSummary) {
    fmt::print(fmt::emphasis::bold, "execution finished with ");
  }

  if ((nError > 0) && (nWarn > 0)) {
    printErrorNum(nError);
    fmt::print(fmt::emphasis::bold, " and ");
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

void TerminalPrinter::onGroupBegin(
    const Entry &entry, const Context &context) const {
  if constexpr (!NoHierarchy) {
    printIndent(context.getDepth());
    fmt::print(fmt::emphasis::italic | fg(GroupColor), "In {}", entry.msg);
    fmt::print(":\n");
  }
}

void TerminalPrinter::onGroupEnd(
    const Entry &entry, const Context &context) const {
  // Do nothing.
}

void TerminalPrinter::onEntry(
    const Entry &entry, const Context &context) const {
  size_t depth;

  if constexpr (!NoHierarchy) {
    depth = context.getDepth();
  } else {
    depth = context.isEmpty() ? 0 : 1;

    auto delimiter = false;
    fmt::print(fmt::emphasis::italic | fg(GroupColor), "In ");
    for (const auto &scope : context.scopes) {
      if (delimiter) fmt::print(" -> ");
      fmt::print(fmt::emphasis::italic | fg(GroupColor), scope);
      delimiter = true;
    }
    fmt::print(":\n");
  }

  const auto lvl = entry.lvl;
  printIndent(depth);
  fmt::print(fg(getSeverityColor(lvl)), "{}: ", getSeverityString(lvl));
  fmt::print("{}\n", entry.msg);
}

} // namespace eda::diag
