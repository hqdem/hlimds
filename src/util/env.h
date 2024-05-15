//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/assert.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace eda::env {

static constexpr const char *uhome = "UTOPIA_HOME";

inline bool isSet(const char *var) {
  return getenv(var);
}

inline std::string getValue(const char *var) {
  const char *value = getenv(var);
  uassert(value, var << " is not set");
  return std::string(value);
}

inline std::filesystem::path getHomePath() {
  return getValue(uhome);
}

inline std::string getHomePathAsString() {
  return getHomePath().string();
}

} // namespace eda::env

