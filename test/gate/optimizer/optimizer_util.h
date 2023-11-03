//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <string>
#include <filesystem>

namespace eda::gate::optimizer {

  const std::string testOutPath = "output/data/gate/optimizer/output";

  std::filesystem::path createOutPath(const std::string &folderName);

} // namespace eda::gate::optimizer
