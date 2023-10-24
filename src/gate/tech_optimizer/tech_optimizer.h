//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

//#include "gate/tech_mapper/tech_mapper.h"

namespace eda::gate::tech_optimizer {
  using GNet = eda::gate::model::GNet;

  void read_db(const std::string &dbPath);
  void tech_optimize(GNet *net, uint approachSelector/*, Constraints &constraints*/);
} // namespace eda::gate::tech_optimizer
