//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"

namespace eda::gate::tech_optimizer {
  using NetID = eda::gate::model::NetID;

  void read_db(const std::string &dbPath);
  void tech_optimize(NetID net, uint approachSelector/*, Constraints &constraints*/);
  void sequence_net(NetID net);
} // namespace eda::gate::tech_optimizer
