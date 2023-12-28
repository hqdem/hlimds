
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/printer/dot.h"

#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/techoptimizer/techmapper.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/premapper/mapper/mapper_test.h"

#include "gtest/gtest.h"


namespace eda::gate::tech_optimizer {

  const std::filesystem::path homePathTechMap = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path libertyDirrectTechMap = homePathTechMap / "test" / "data" / "gate" / "tech_mapper";

  TEST(TechMapTest, RandomSubnet) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

  SubnetID subnetID = model::randomSubnet(10, 3, 20, 1, 2);
    Techmaper techmaper;

    techmaper.setLiberty(libertyDirrectTechMap.string() +
        "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    techmaper.setMapper(Techmaper::TechmaperType::FUNC);
    techmaper.setStrategy(Techmaper::TechmaperStrategyType::SIMPLE);

    techmaper.techmap(subnetID);
  }
/*
  TEST(TechMapTest, DISABLED_gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet2(net);

    CutBasedTechMapper cutBasedTechMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    cutBasedTechMapper.techMap(&net, minDelay, false);
  }

  TEST(TechMapTest, DISABLED_gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet3(net);

    CutBasedTechMapper cutBasedTechMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    cutBasedTechMapper.techMap(&net, minDelay, false);
  }

  TEST(TechMapTest, DISABLED_c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c432.v");

    CutBasedTechMapper cutBasedTechMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    cutBasedTechMapper.techMap(net, minDelay, true);
  }

  TEST(TechMapTest, DISABLED_adder) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("adder.v");

    CutBasedTechMapper cutBasedTechMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    cutBasedTechMapper.techMap(net, minDelay, true);
  }

  TEST(TechMapTest,  DISABLED_c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c17.v");

    CutBasedTechMapper cutBasedTechMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    cutBasedTechMapper.techMap(net, minDelay, true);
  }
  */
}