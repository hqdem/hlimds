//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/parser/parser_test.h"
#include "gate/printer/dot.h"

#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace eda::gate::optimizer {

  using GateIdList = eda::gate::model::GNet::GateIdList;
  using eda::gate::parser::parseVerilog;

  /* in1   in2                          */
  /* ┌─┐   ┌─┐                          */
  /* └─┘─┐ └─┘─┐                        */
  /* ┌─┐ |_┌─┐ |_┌─┐                    */
  /* └─┘───└─┘───└─┘─┐                  */
  /* in0  and4   and5|                  */
  /*             ┌─┐ |_┌─┐              */
  /*             └─┘───└─┘              */
  /*             in3   or6              */
  std::vector<GateId> andOr(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &elem : g) {
      elem = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {2, 4}));
    g.push_back(createLink(gNet, g, {3, 5}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  TEST(DotPrinter, andOr) {
    std::string graphFileName = "andOr.dot";
    std::string homePath = std::string(std::getenv("UTOPIA_HOME"));
    std::string fileDir = homePath + "/output/test/printer/";
    std::string command = "mkdir -p " + fileDir;
    std::system(command.c_str());
    GNet net;
    andOr(net);
    Dot dot(&net);
    dot.print(fileDir + graphFileName);
    bool b = (std::filesystem::exists(fileDir + graphFileName) &&
              !std::filesystem::is_empty(fileDir + graphFileName));
    EXPECT_TRUE(b);
  }

  TEST(DotPrinter, c17) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c17.v"));
    const auto &gate = gNet.gates();
    GateIdList list = {gate[0]->id(), gate[7]->id(), gate[9]->id()};
    std::string graphFileName = "c17.dot";
    std::string homePath = std::string(std::getenv("UTOPIA_HOME"));
    std::string fileDir = homePath + "/output/test/printer/";
    std::string command = "mkdir -p " + fileDir;
    std::system(command.c_str());
    Dot dot(&gNet);
    dot.fillColorGate(fileDir + graphFileName, list);
    bool b = (std::filesystem::exists(fileDir + graphFileName) &&
              !std::filesystem::is_empty(fileDir + graphFileName));
    EXPECT_TRUE(b);
  }
} // namespace eda::gate::optimizer
