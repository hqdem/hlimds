//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <gate/model/object.h>

#include <string>
#include <vector>

namespace eda::gate::translator {

model::CellTypeID
readVerilogDesign(
    const std::string &top, const std::vector<std::string> &files);

} // namespace eda::gate::translator
