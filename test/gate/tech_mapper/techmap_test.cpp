
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
  void dump(const GNet &net) {
  std::cout << net << '\n';

  for (auto source : net.sourceLinks()) {
    const auto *gate = Gate::get(source.target);
    std::cout << *gate << std::endl;
  }
  for (auto target : net.targetLinks()) {
    const auto *gate = Gate::get(target.source);
    std::cout << *gate << std::endl;
  }

  std::cout << std::endl;
  std::cout << "N=" << net.nGates() << '\n';
  std::cout << "I=" << net.nSourceLinks() << '\n';
  std::cout << "O=" << net.nTargetLinks() << '\n';
}

  using GNet = eda::gate::model::GNet;
  using lorina::text_diagnostics;
  using lorina::diagnostic_engine;
  using lorina::return_code;

  const std::filesystem::path homePathTechMap = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path libertyDirrectTechMap = homePathTechMap / "test" / "data" / "gate" / "tech_mapper";

  std::string netPath(const std::string &nameDir) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath =
            homePath / "test/data/gate/tech_mapper/output" / nameDir;
    system(std::string("mkdir -p ").append(outputPath).c_str());
    system(std::string("mkdir -p ").append(outputPath / "before").c_str());
    return outputPath;
  }

  TEST(TechMapTest, gnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet1(net);

    dump(net);
    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay);

     dump(net);
  }

  TEST(TechMapTest, gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet2(net);


    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay);
  }
  TEST(TechMapTest, gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet net;
    gnet3(net);

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(&net, minDelay);
  }

  TEST(TechMapTest, c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c432.v");
    
    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();
    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    GNet *gnet = premapped.get();

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(gnet, minDelay);
  }

  TEST(TechMapTest, adder) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("adder.v");

    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();
    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    GNet *gnet = premapped.get();

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(gnet, minDelay);
  }

  TEST(TechMapTest, c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet *net = parseVerilog("c17.v");

    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();
    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    GNet *gnet = premapped.get();

    TechMapper techMapper(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
    
    MinDelay *minDelay = new MinDelay();
    techMapper.techMap(gnet, minDelay);
  }
}
