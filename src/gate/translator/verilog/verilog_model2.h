//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/translator/firrtl.h"

#include <string>

namespace eda::gate::model {

int translateToModel2(const FirrtlConfig &firrtlConfig);
} // namespace eda::gate::model