//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

#include "gate/translator/firrtl.h"

#include "options.h"

namespace eda::gate::model {

struct Model2Config {
    std::string outNetFileName;
    std::vector<std::string> files;
};

int translateToModel2(const FirrtlConfig &firrtlConfig,
                      const Model2Config &model2Config);
} // namespace eda::gate::model