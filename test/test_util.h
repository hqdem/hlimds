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

inline std::filesystem::path getHomePath() {
  uassert(getenv(uhome), uhome << " is not set.");
  return std::string(getenv(uhome));
}

inline std::filesystem::path createOutDir(const std::string &folderName) {
  std::filesystem::path homePath = getHomePath();
  std::filesystem::path outputPath = homePath / "build" / folderName;

  std::filesystem::create_directories(outputPath);

  return outputPath;
}
