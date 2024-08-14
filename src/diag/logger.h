//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

#include <fmt/format.h>

#include <cassert>
#include <sstream>

#define UTOPIA_DIAGNOSE(logger, lvl, msg)\
  do {\
    std::stringstream out;\
    out << msg;\
    eda::diag::log(logger, lvl, out.str());\
  } while(false)

#define UTOPIA_LOG_NOTE(logger, msg)\
  UTOPIA_DIAGNOSE(logger, eda::diag::NOTE, msg)
#define UTOPIA_LOG_WARN(logger, msg)\
  UTOPIA_DIAGNOSE(logger, eda::diag::WARN, msg)
#define UTOPIA_LOG_ERROR(logger, msg)\
  UTOPIA_DIAGNOSE(logger, eda::diag::ERROR, msg)
#define UTOPIA_LOG_BEGIN(logger, msg)\
  UTOPIA_DIAGNOSE(logger, eda::diag::BEGIN, msg)
#define UTOPIA_LOG_END(logger)\
  UTOPIA_DIAGNOSE(logger, eda::diag::END, "")

#define UTOPIA_LOGGER eda::diag::Logger::getDefault()

#define UTOPIA_NOTE(msg)  UTOPIA_LOG_NOTE(UTOPIA_LOGGER, msg)
#define UTOPIA_WARN(msg)  UTOPIA_LOG_WARN(UTOPIA_LOGGER, msg)
#define UTOPIA_ERROR(msg) UTOPIA_LOG_ERROR(UTOPIA_LOGGER, msg)
#define UTOPIA_BEGIN(msg) UTOPIA_LOG_BEGIN(UTOPIA_LOGGER, msg)
#define UTOPIA_END()      UTOPIA_LOG_END(UTOPIA_LOGGER, msg)

namespace eda::diag {

class Logger {
public:
  static Logger &getDefault() {
    static Logger logger;
    return logger;
  }

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

#if __cplusplus >= 202002L
  template <typename... T>
  inline void log(Logger &logger,
                  Severity lvl, fmt::format_string<T...> fmt, T&&... args) {
    // W/ compile-time checks of the format string.
    vlog(logger, lvl, fmt, fmt::make_format_args(args...));
  }
#else
  template <typename... T>
  inline void log(Logger &logger,
                  Severity lvl, fmt::string_view fmt, T&&... args) {
    // W/o compile-time checks of the format string.
    vlog(logger, lvl, fmt, fmt::make_format_args(args...));
  }
#endif // C++20

} // namespace eda::diag
