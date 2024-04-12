//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

#include "gate/techmapper/techmapper.h"
#include "options.h"

namespace eda::gate::techmapper {

struct TechMapConfig {
  std::string outNetFileName;
  Techmapper::MapperType type;
  std::vector<std::string> files;
};

int techMap(TechMapConfig config);

} // namespace  eda::gate::techmapper