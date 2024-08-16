//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/diagnostics.h"

#include <easylogging++.h>
#include <fmt/format.h>

#include <cassert>
#include <iostream>
#include <sstream>

#define UTOPIA_OUT std::cout
#define UTOPIA_ERR std::cerr

// Diagnostics.
#define UTOPIA_RAISE_DIAGNOSTICS(logger, lvl, msg)\
  do {\
    std::stringstream out;\
    out << msg;\
    eda::diag::log(logger, lvl, out.str());\
  } while(false)

#define UTOPIA_RAISE_NOTE(logger, msg)\
  UTOPIA_RAISE_DIAGNOSTICS(logger, eda::diag::NOTE, msg)
#define UTOPIA_RAISE_WARN(logger, msg)\
  UTOPIA_RAISE_DIAGNOSTICS(logger, eda::diag::WARN, msg)
#define UTOPIA_RAISE_ERROR(logger, msg)\
  UTOPIA_RAISE_DIAGNOSTICS(logger, eda::diag::ERROR, msg)
#define UTOPIA_RAISE_BEGIN(logger, msg)\
  UTOPIA_RAISE_DIAGNOSTICS(logger, eda::diag::BEGIN, msg)
#define UTOPIA_RAISE_END(logger)\
  UTOPIA_RAISE_DIAGNOSTICS(logger, eda::diag::END, "")

#define UTOPIA_LOGGER eda::diag::Logger::getDefault()

#define UTOPIA_NOTE(msg)  UTOPIA_RAISE_NOTE(UTOPIA_LOGGER, msg)
#define UTOPIA_WARN(msg)  UTOPIA_RAISE_WARN(UTOPIA_LOGGER, msg)
#define UTOPIA_ERROR(msg) UTOPIA_RAISE_ERROR(UTOPIA_LOGGER, msg)
#define UTOPIA_BEGIN(msg) UTOPIA_RAISE_BEGIN(UTOPIA_LOGGER, msg)
#define UTOPIA_END()      UTOPIA_RAISE_END(UTOPIA_LOGGER, msg)

// Logging.
#define UTOPIA_INFO(msg)  LOG(INFO) << msg
#define UTOPIA_DEBUG(msg) LOG(DEBUG) << msg

#define UTOPIA_INITIALIZE_LOGGER()\
  el::Loggers::reconfigureAllLoggers(\
      el::ConfigurationType::Format, "%level %datetime{%H:%m:%s}: %msg");

namespace eda::diag {

class Logger {
public:
  static Logger &getDefault() {
    static Logger logger;
    return logger;
  }

  Logger(): locallyCreated(true), diagnostics(new Diagnostics()) {}
  Logger(Diagnostics &diagnostics): diagnostics(&diagnostics) {}

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  virtual ~Logger() {
    if (locallyCreated && diagnostics) {
      delete diagnostics;
      diagnostics = nullptr /* paranoid */;
    }
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
