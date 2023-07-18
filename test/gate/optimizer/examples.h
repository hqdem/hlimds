//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/parser/gate_verilog_parser.h"

#include <filesystem>
#include <string>

/**
 * \brief Synthetic nets examples.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
namespace eda::gate::optimizer {

  GateID createLink(GNet &gNet, const std::vector<GateID> &g,
                    const std::vector<GateID> &input,
                    model::GateSymbol func = model::GateSymbol::Value::AND);

  std::vector<GateID> gnet1(GNet &gNet) ;

  std::vector<GateID> gnet1Extended(GNet &gNet);

  std::vector<GateID> gnet2(GNet &gNet);

  std::vector<GateID> gnet2Extended(GNet &gNet);

  std::vector<GateID> gnet3(GNet &gNet);

  std::vector<GateID> gnet3Cone(GNet &gNet);

  std::vector<GateID> gnet4(GNet &gNet);

  Visitor::MatchMap createPrimitiveMap(GNet *subNet,  const Cut &cut);

} // namespace eda::gate::optimizer

