//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/env.h"

inline std::filesystem::path createOutDir(const std::string &folderName) {
  std::filesystem::path homePath = eda::env::getHomePath();
  std::filesystem::path outputPath = homePath / "output" / folderName;

  std::filesystem::create_directories(outputPath);

  return outputPath;
}
