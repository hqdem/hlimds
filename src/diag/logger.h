//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

#include <fmt/core.h>
#include <fmt/format.h>

#include <cassert>
#include <sstream>

#define DIAGNOSE(logger, lvl, msg)\
  do {\
    std::stringstream out;\
    out << msg;\
    eda::diag::log(logger, lvl, out.str());\
  } while(false)

#define DIAGNOSE_NOTE(logger, msg)  DIAGNOSE(logger, eda::diag::NOTE, msg)
#define DIAGNOSE_WARN(logger, msg)  DIAGNOSE(logger, eda::diag::WARN, msg)
#define DIAGNOSE_ERROR(logger, msg) DIAGNOSE(logger, eda::diag::ERROR, msg)

namespace eda::diag {

class Logger {
public:
  Logger(): locallyCreated(true), diagnostics(new Diagnostics()) {}
  Logger(Diagnostics &diagnostics): diagnostics(&diagnostics) {}

  virtual ~Logger() {
    if (locallyCreated) delete diagnostics;
  }

  virtual void log(const Entry &entry) {
    diagnostics->add(entry);
  }

  const Diagnostics &getDiagnostics() const {
    assert(diagnostics);
    return *diagnostics;
  }

  Diagnostics &getDiagnostics() {
    assert(diagnostics);
    return *diagnostics;
  }

private:
  const bool locallyCreated{false};
  Diagnostics *diagnostics{nullptr};
};

inline void vlog(Logger &logger,
                 Severity lvl, fmt::string_view fmt, fmt::format_args args) {
  const Entry entry{lvl, fmt::vformat(fmt, args)};
  logger.log(entry);
}

template <typename... T>
inline void log(Logger &logger,
                Severity lvl, fmt::string_view fmt, T&&... args) {
  vlog(logger, lvl, fmt, fmt::make_format_args(args...));
}

} // namespace eda::diag
