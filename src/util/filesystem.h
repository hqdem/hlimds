//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <filesystem>
#include <string>

using std::filesystem::path;

namespace eda::util {

inline bool createDir(const path &dir) {
  if (!std::filesystem::exists(dir)) {
    return std::filesystem::create_directories(dir);
  }
  return true;
}

inline bool createDir(const std::string &name) {
  path dir{name};
  return createDir(dir);
}

} // namespace eda::util
