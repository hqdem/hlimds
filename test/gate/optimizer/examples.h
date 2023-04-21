//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {

  GateID createLink(GNet &gNet, const std::vector<GateID> &g,
                    const std::vector<GateID> &input,
                    model::GateSymbol func = model::GateSymbol::Value::AND);

  std::vector<GateID> gnet1(GNet &gNet) ;

  std::vector<GateID> gnet1Exteded(GNet &gNet);

  std::vector<GateID> gnet2(GNet &gNet);

  std::vector<GateID> gnet3(GNet &gNet);

  std::vector<GateID> gnet3Cone(GNet &gNet);

  std::vector<GateID> gnet4(GNet &gNet);

  std::unordered_map<GateID, GateID> createPrimitiveMap(GNet *subNet,  const Cut &cut);
} // namespace eda::gate::optimizer

