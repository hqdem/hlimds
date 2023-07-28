//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer_util.h"

namespace eda::gate::optimizer {

  std::filesystem::path createOutPath(const std::string &folderName) {
    std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    std::filesystem::path outputPath =
            homePath / "build" / testOutPath / folderName;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    return outputPath;
  }

} // namespace eda::gate::optimizer