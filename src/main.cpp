//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/debugger/base_checker.h"
#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "gate/optimizer/rwmanager.h"
#include "gate/premapper/aigmapper.h"
#include "gate/premapper/migmapper.h"
#include "gate/premapper/premapper.h"
#include "gate/premapper/xagmapper.h"
#include "gate/premapper/xmgmapper.h"
#include "gate/printer/graphml.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/translator/fir_to_model2/fir_to_model2_wrapper.h"
#include "gate/translator/firrtl.h"
#include "options.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/arithmetic.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/parser.h"
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

void dump(const GNet &net) {
  std::cout << net << std::endl;

  for (auto source : net.sourceLinks()) {
    const auto *gate = RtlContext::Gate::get(source.target);
    std::cout << *gate << std::endl;
  }
  for (auto target : net.targetLinks()) {
    const auto *gate = RtlContext::Gate::get(target.source);
    std::cout << *gate << std::endl;
  }

  std::cout << std::endl;
  std::cout << "N=" << net.nGates() << std::endl;
  std::cout << "I=" << net.nSourceLinks() << std::endl;
  std::cout << "O=" << net.nTargetLinks() << std::endl;
}

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

bool compile(RtlContext &context) {
  LOG(INFO) << "RTL compile";

  RtlContext::Compiler compiler(RtlContext::Library::get());
  context.gnet0 = compiler.compile(*context.vnet);

  if (context.gnet0 == nullptr) {
    LOG(ERROR) << "Could not compile the model";
    return false;
  }

  context.gnet0->sortTopologically();

  std::cout << "------ G-net #0 ------" << std::endl;
  dump(*context.gnet0);

  return true;
}

bool premap(RtlContext &context) {
  LOG(INFO) << "RTL premap";

  auto &premapper =
      eda::gate::premapper::getPreMapper(context.options.preBasis);
  context.gnet1 = premapper.map(*context.gnet0, context.gmap);

  context.gnet1->sortTopologically();
  std::cout << "------ G-net #1 ------" << std::endl;
  dump(*context.gnet1);

  return true;
}

bool check(RtlContext &context) {

  LOG(INFO) << "RTL check";

  auto &checker = eda::gate::debugger::getChecker(context.options.lecType);

  assert(context.gnet0->nSourceLinks() == context.gnet1->nSourceLinks());
  assert(context.gnet0->nTargetLinks() == context.gnet1->nTargetLinks());

  context.equal = checker.areEqual(*context.gnet0, *context.gnet1,
                                    context.gmap);
  std::cout << "equivalent=" << context.equal << std::endl;

  return true;
}

bool print(RtlContext &context) {
  std::ofstream fout;
  fout.open(context.options.graphMl);
  eda::printer::graphml::GraphMlPrinter::print(fout, *context.gnet1);
  fout.close();
  return true;
}

int rtlMain(RtlContext &context) {
  if (!initialize(context)) { return -1; }
  if (!parse(context))      { return -1; }
  if (!compile(context))    { return -1; }
  if (!premap(context))     { return -1; }
  if (!check(context))      { return -1; }
  if (!print(context))      { return -1; }

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

    if (options.rtl.files().empty() &&
        options.firrtl.files().empty() &&
        options.model2.files().empty() &&
        options.techMapOptions.files().empty()) {
      throw CLI::CallForAllHelp();
    }
  }
   catch(const CLI::ParseError &e) {
    return options.exit(e);
  }

  int result = 0;

  for (auto file : options.rtl.files()) {
    RtlContext context(file, options.rtl);
    result |= rtlMain(context);
  }

  if (!options.firrtl.files().empty()) {
    FirrtlConfig cfg;
    FirRtlOptions &opts = options.firrtl;
    cfg.debugMode = opts.debugMode;
    cfg.outputNamefile = opts.outputNamefile;
    cfg.topModule = opts.top;
    cfg.files = opts.files();
    result |= translateToFirrtl(cfg);
  }

  if (!options.model2.files().empty()) {
    Model2Options &opts = options.model2;
    FirrtlConfig firrtlConfig;
    firrtlConfig.debugMode = opts.debugMode;
    firrtlConfig.outputNamefile = opts.firrtlFileName;
    firrtlConfig.topModule = opts.topModuleName;
    firrtlConfig.files = opts.files();
    Model2Config model2Config;
    model2Config.outNetFileName = opts.outNetFileName;
    model2Config.files = opts.files();
    result |= translateToModel2(firrtlConfig, model2Config);
  }

  if(!options.techMapOptions.files().empty()){
    TechMapOptions &opts = options.techMapOptions;
    eda::gate::tech_optimizer::TechMapConfig config;
    config.files = opts.files();
    config.outNetFileName = opts.outputPath;
    config.type = opts.mapperType;
    result |= eda::gate::tech_optimizer::techMap(config);
  }
  
  return result;
}