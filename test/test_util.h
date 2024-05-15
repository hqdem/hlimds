//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/assert.h"

#include <filesystem>
#include <string>

static constexpr const char *uhome = "UTOPIA_HOME";

inline std::string getEnv(const char *var) {
  uassert(getenv(var), var << " is not set.");
  return std::string(getenv(var));
}

inline std::filesystem::path getHomePath() {
  return getEnv(uhome);
}

inline std::string getHomePathAsString() {
  return getHomePath().string();
}

inline std::filesystem::path createOutDir(const std::string &folderName) {
  std::filesystem::path homePath = getHomePath();
  std::filesystem::path outputPath = homePath / "build" / folderName;

  std::filesystem::create_directories(outputPath);

  return outputPath;
}
