//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/analyzer/simulation_estimator.h"
#include "gate/debugger/sat_checker.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/area_optimizer.h"
#include "gate/parser/graphml_parser.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/translator/firrtl.h"
#include "gate/translator/model2.h"
#include "gate/translator/verilog/verilog_model2.h"
#include "options.h"
/*
#include "rtl/parser/ril/parser.h"
*/
#include "util/string.h"

#include "easylogging++.h"

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

INITIALIZE_EASYLOGGINGPP

//===-----------------------------------------------------------------------===/
// Logic Synthesis
//===-----------------------------------------------------------------------===/

/*
struct RtlContext {
  using VNet = eda::rtl::model::Net;
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using Link = Gate::Link;

  using AigMapper = eda::gate::premapper::AigMapper;
  using Checker = eda::gate::debugger::BaseChecker;
  using Compiler = eda::rtl::compiler::Compiler;
  using Library = eda::rtl::library::ArithmeticLibrary;
  using MigMapper = eda::gate::premapper::MigMapper;
  using PreBasis = eda::gate::premapper::PreBasis;
  using PreMapper = eda::gate::premapper::PreMapper;
  using RwManager = eda::gate::optimizer::RewriteManager;
  using XagMapper = eda::gate::premapper::XagMapper;
  using XmgMapper = eda::gate::premapper::XmgMapper;

  RtlContext(const std::string &file, const RtlOptions &options):
    file(file), options(options) {}

  const std::string file;
  const RtlOptions &options;

  std::shared_ptr<VNet> vnet;
  std::shared_ptr<GNet> gnet0;
  std::shared_ptr<GNet> gnet1;

  PreMapper::GateIdMap gmap;

  bool equal;
};

bool initialize(RtlContext &context) {
  LOG(INFO) << "RTL initialize";

  RtlContext::RwManager::get().initialize(context.options.preLib);
  return true;
}

bool parse(RtlContext &context) {
  LOG(INFO) << "RTL parse: " << context.file;

  context.vnet = eda::rtl::parser::ril::parse(context.file);

  if (context.vnet == nullptr) {
    LOG(ERROR) << "Could not parse the file";
    return false;
  }

  std::cout << "------ P/V-nets ------" << std::endl;
  std::cout << *context.vnet << std::endl;

  return true;
}

int rtlMain(RtlContext &context) {
  if (!initialize(context)) { return -1; }
  if (!parse(context))      { return -1; }

  return 0;
}
*/

eda::gate::model::SubnetID parseGraphML(const std::string &fileName) {
  uassert(std::filesystem::exists(fileName), "File doesn't exist" << std::endl);

  eda::gate::parser::graphml::GraphMlParser parser;
  return parser.parse(fileName).make();
}

int optimize(const eda::gate::model::SubnetID &oldSubnetId,
             GraphMlOptions &opts) {
  using SatChecker = eda::gate::debugger::SatChecker;
  using AreaOptimizer = eda::gate::optimizer::AreaOptimizer;

  const auto &oldSubnet = eda::gate::model::Subnet::get(oldSubnetId);
  eda::gate::analyzer::SimulationEstimator estimator;
  size_t depthBefore = oldSubnet.getPathLength().second;
  size_t sizeBefore = oldSubnet.getEntries().size();
  size_t powerBefore = 
      estimator.estimate(oldSubnet).getSwitchProbabilitiesSum();
  eda::gate::model::SubnetBuilder subnetBuilder;
  auto inputs = subnetBuilder.addInputs(oldSubnet.getInNum());
  auto outputs = subnetBuilder.addSubnet(oldSubnetId, inputs);
  subnetBuilder.addOutputs(outputs);

  if (opts.optCrit == eda::gate::optimizer::Area) {
    AreaOptimizer areaOptimizer(subnetBuilder, 2, 10);
    areaOptimizer.optimize();
  } else if (opts.optCrit == eda::gate::optimizer::Delay) {
    //TODO Depth
  } else {
    //TODO Switching Activity
  }

  const auto &newSubnetId = subnetBuilder.make();
  const auto &newSubnet = eda::gate::model::Subnet::get(newSubnetId);
  size_t depthAfter = newSubnet.getPathLength().second;
  size_t powerAfter = estimator.estimate(newSubnet).getSwitchProbabilitiesSum();

  if (opts.lec) {
    SatChecker &checker = SatChecker::get();

    std::unordered_map<size_t, size_t> map;
    for (size_t i{0}; i < oldSubnet.getInNum(); ++i) {
      map[i] = i;
    }

    for (int c = oldSubnet.getOutNum(); c > 0; --c) {
      map[oldSubnet.size() - c] = newSubnet.size() - c;
    }
    bool flag = checker.areEquivalent(oldSubnetId, newSubnetId, map).equal();
    std::cout << "Equivalence: " << flag << std::endl;

    if (!flag) {
      return -1;
    }
  }

  std::cout << "Size before: " << sizeBefore << std::endl;
  std::cout << "Size after: " << newSubnet.getEntries().size() << std::endl;
  std::cout << "Depth before: " << depthBefore << std::endl;
  std::cout << "Depth after: " << depthAfter << std::endl;
  std::cout << "Power before: " << powerBefore << std::endl;
  std::cout << "Power after: " << powerAfter << std::endl;
  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  std::stringstream title;
  std::stringstream version;

  version << VERSION_MAJOR << "." << VERSION_MINOR;

  title << "Utopia EDA " << version.str() << " | ";
  title << "Copyright (c) " << YEAR_STARTED << "-" << YEAR_CURRENT << " ISPRAS";

  Options options(title.str(), version.str());

  try {
    options.initialize("config.json", argc, argv);

    if (/*options.rtl.files().empty() &&*/
        options.firrtl.files().empty() &&
        options.model2.files().empty() &&
        options.graphMl.files().empty() &&
        options.techMapOptions.files().empty() &&
        options.verilogToModel2.files().empty()) {
      throw CLI::CallForAllHelp();
    }
  }
   catch(const CLI::ParseError &e) {
    return options.exit(e);
  }

  int result = 0;

/*
  for (auto file : options.rtl.files()) {
    RtlContext context(file, options.rtl);
    result |= rtlMain(context);
  }
*/

  if (!options.firrtl.files().empty()) {
    FirrtlConfig cfg;
    FirRtlOptions &opts = options.firrtl;
    cfg.debugMode = opts.debugMode;
    cfg.outputFileName = opts.outputNamefile;
    cfg.topModule = opts.top;
    cfg.files = opts.files();
    result |= translateToFirrtl(cfg);
  }

  if (!options.verilogToModel2.files().empty()) {
    YosysToModel2Config cfg;
    YosysToModel2Options &opts = options.verilogToModel2;
    cfg.debugMode = opts.debugMode;
    cfg.topModule = opts.top;
    cfg.files = opts.files();
    result |= translateVerilogToModel2(cfg);
  }

  if (!options.model2.files().empty()) {
    Model2Options &opts = options.model2;
    FirrtlConfig firrtlConfig;
    firrtlConfig.debugMode = opts.verbose;
    firrtlConfig.outputFileName = opts.outFileName;
    firrtlConfig.topModule = opts.top;
    firrtlConfig.files = opts.files();
    result |= eda::gate::model::translateToModel2(firrtlConfig);
  }

  GraphMlOptions &opts = options.graphMl;
  if (!options.graphMl.files().empty() &&
      (opts.optCrit != eda::gate::optimizer::NoOpt)) {
    for (const auto &file : options.graphMl.files()) {
      const auto &oldSubnetId = parseGraphML(file);
      result |= optimize(oldSubnetId, options.graphMl);
    }
  }

  if(!options.techMapOptions.files().empty()){
    TechMapOptions &opts = options.techMapOptions;
    eda::gate::techmapper::TechMapConfig config;
    config.files = opts.files();
    config.outNetFileName = opts.outputPath;
    config.type = opts.mapperType;
    result |= eda::gate::techmapper::techMap(config);
  }
  
  return result;
}
