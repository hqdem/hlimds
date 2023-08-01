//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/parser/gate_verilog.h"

#include <filesystem>
#include <string>

/**
 * \brief Synthetic nets examples.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
namespace eda::gate::optimizer {

  GateId createLink(GNet &gNet, const std::vector<GateId> &g,
                    const std::vector<GateId> &input,
                    model::GateSymbol func = model::GateSymbol::Value::AND);

  std::vector<GateId> gnet1(GNet &gNet) ;

  std::vector<GateId> gnet1Extended(GNet &gNet);

  std::vector<GateId> gnet1ChangedFunc(GNet &gNet);

  std::vector<GateId> gnet2(GNet &gNet);

  std::vector<GateId> gnet2Extended(GNet &gNet);

  std::vector<GateId> gnet3(GNet &gNet);

  std::vector<GateId> gnet3Cone(GNet &gNet);

  std::vector<GateId> gnet4(GNet &gNet);

  Visitor::MatchMap createPrimitiveMap(GNet *subNet,  const Cut &cut);

} // namespace eda::gate::optimizer

