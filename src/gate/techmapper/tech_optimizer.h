//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

namespace eda::gate::techmapper {

using GNet = eda::gate::model::GNet;

void read_db(const std::string &dbPath);
void tech_optimize(GNet *net, uint approachSelector/*, Constraints &constraints*/);

} // namespace eda::gate::techmapper
