//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"

#include <cmath>
#include <ostream>

namespace eda::gate::model {

std::ostream &operator <<(std::ostream &out, const DesignBuilder &builder);

} // namespace eda::gate::model
