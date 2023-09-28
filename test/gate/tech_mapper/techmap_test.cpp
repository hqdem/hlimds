
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/printer/dot.h"
#include "gate/optimizer/examples.h"

#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/tech_mapper/tech_map.h"
#include "gate/premapper/mapper/mapper_test.h"
#include "gate/debugger/checker.h"
#include "gate/tech_mapper/strategy/min_delay.h"
#include "gate/tech_mapper/strategy/strategy.h"

#include "gtest/gtest.h"

using namespace eda::gate::parser;
using namespace eda::gate::optimizer;

namespace eda::gate::techMap {

  using GNet = eda::gate::model::GNet;
  using lorina::text_diagnostics;
  using lorina::diagnostic_engine;
  using lorina::return_code;

  const std::filesystem::path homePathTechMap = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path libertyDirrectTechMap = homePathTechMap / "test" / "data" / "gate" / "tech_mapper";

  TEST(TechMapTest, gnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet1(net);

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay, false);
  }

  TEST(TechMapTest, DISABLED_gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet2(net);


    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay, false);
  }

  TEST(TechMapTest, DISABLED_gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet3(net);

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay, false);
  }

  TEST(TechMapTest, DISABLED_c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c432.v");

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(net, minDelay, true);
  }

  TEST(TechMapTest, DISABLED_adder) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("adder.v");

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(net, minDelay, true);
  }

  TEST(TechMapTest, DISABLED_c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c17.v");

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(net, minDelay, true);
  }
}
