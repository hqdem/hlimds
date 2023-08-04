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

  std::vector<GateID> balanceAND(GNet &gNet);

  std::vector<GateID> balanceANDTwice(GNet &gNet);

  std::vector<GateID> balanceANDThrice(GNet &gNet);

  std::vector<GateID> oneInOneOut(GNet &gNet);

  std::vector<GateID> unbalancableANDOR(GNet &gNet);

  std::vector<GateID> balanceOR(GNet &gNet);

  std::vector<GateID> balanceXORXNOR(GNet &gNet);

  std::vector<GateID> balanceSeveralOut(GNet &gNet);

} // namespace eda::gate::optimizer

