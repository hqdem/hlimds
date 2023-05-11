//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/debugger/checker.h"
#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "gate/optimizer/rwmanager.h"
#include "gate/premapper/aigmapper.h"
#include "gate/premapper/migmapper.h"
#include "gate/premapper/premapper.h"
#include "gate/premapper/xagmapper.h"
#include "gate/premapper/xmgmapper.h"
#include "hls/compiler/compiler.h"
#include "hls/mapper/mapper.h"
#include "hls/model/model.h"
#include "hls/model/printer.h"
#include "hls/parser/hil/parser.h"
#include "hls/scheduler/latency_solver.h"
#include "hls/scheduler/param_optimizer.h"
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
  using Checker = eda::gate::debugger::Checker;
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
    LOG(ERROR) << "Could not parse the file";;
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
  using Link = RtlContext::Link;
  using GateBinding = RtlContext::Checker::GateBinding;

  LOG(INFO) << "RTL check";

  RtlContext::Checker checker;
  GateBinding ibind, obind, tbind;

  assert(context.gnet0->nSourceLinks() == context.gnet1->nSourceLinks());
  assert(context.gnet0->nTargetLinks() == context.gnet1->nTargetLinks());

  // Input-to-input correspondence.
  for (auto oldSourceLink : context.gnet0->sourceLinks()) {
    auto newSourceId = context.gmap[oldSourceLink.target];
    ibind.insert({oldSourceLink, Link(newSourceId)});
  }

  // Output-to-output correspondence.
  for (auto oldTargetLink : context.gnet0->targetLinks()) {
    auto newTargetId = context.gmap[oldTargetLink.source];
    obind.insert({oldTargetLink, Link(newTargetId)});
  }

  // Trigger-to-trigger correspondence.
  for (auto oldTriggerId : context.gnet0->triggers()) {
    auto newTriggerId = context.gmap[oldTriggerId];
    tbind.insert({Link(oldTriggerId), Link(newTriggerId)});
  }

  RtlContext::Checker::Hints hints;
  hints.sourceBinding  = std::make_shared<GateBinding>(std::move(ibind));
  hints.targetBinding  = std::make_shared<GateBinding>(std::move(obind));
  hints.triggerBinding = std::make_shared<GateBinding>(std::move(tbind));

  context.equal = checker.areEqual(*context.gnet0, *context.gnet1, hints);
  std::cout << "equivalent=" << context.equal << std::endl;

  return true;
}

int rtlMain(RtlContext &context) {
  if (!initialize(context)) { return -1; }
  if (!parse(context))      { return -1; }
  if (!compile(context))    { return -1; }
  if (!premap(context))     { return -1; }
  if (!check(context))      { return -1; }

  return 0;
}

//===-----------------------------------------------------------------------===/
// High-Level Synthesis
//===-----------------------------------------------------------------------===/

struct HlsContext {
  using Model = eda::hls::model::Model;
  using Indicators = eda::hls::model::Indicators;
  using Parameters = eda::hls::model::Parameters;
  using Criteria = eda::hls::model::Criteria;
  template<typename T>
  using Constraint = eda::hls::model::Constraint<T>;

  using Mapper = eda::hls::mapper::Mapper;
  using Library = eda::hls::library::Library;
  using Method = eda::hls::scheduler::LatencyLpSolver;
  using Optimizer = eda::hls::scheduler::ParametersOptimizer<Method>;
  using Compiler = eda::hls::compiler::Compiler;

  HlsContext(const std::string &file, const HlsOptions &options):
    file(file),
    options(options),
    criteria(
        Indicator::PERF,
        Constraint<unsigned>(40000, 500000), // Frequency (kHz)
        Constraint<unsigned>(1000, 500000),  // Performance (=frequency)
        Constraint<unsigned>(0, 1000),       // Latency (cycles)
        Constraint<unsigned>(),              // Power (does not matter)
        Constraint<unsigned>(1, 10000000))   // Area (number of LUTs)
    {}

  const std::string file;
  const HlsOptions &options;

  Criteria criteria;
  std::shared_ptr<Model> model;
  Indicators indicators;
  std::map<std::string, Parameters> params;
};

bool parse(HlsContext &context) {
  LOG(INFO) << "HLS parse: " << context.file;

  context.model = eda::hls::parser::hil::parse(context.file);

  if (context.model == nullptr) {
    LOG(ERROR) << "Could not parse the file";
    return false;
  }

  std::cout << "------ HLS model #0 ------" << std::endl;
  std::cout << *context.model << std::endl;

  return true;
}

bool optimize(HlsContext &context) {
  context.model->save();

  // Map the model nodes to meta elements.
  HlsContext::Mapper::get().map(*context.model, HlsContext::Library::get());
  // Optimize the model.
  context.params = HlsContext::Optimizer::get().optimize(
    context.criteria, *context.model, context.indicators);

  context.model->save();

  std::cout << "------ HLS model #1 ------" << std::endl;
  std::cout << *context.model << std::endl;

  if (!context.options.outDot.empty()) {
    std::ofstream output(context.options.outDir + "/" + context.options.outDot);
    eda::hls::model::printDot(output, *context.model);
    output.close();
  }

  return true;
}

bool compile(HlsContext &context) {
  auto compiler = std::make_unique<HlsContext::Compiler>();

  auto circuit = compiler->constructFirrtlCircuit(*context.model, "main");
  circuit->printFiles(context.options.outMlir,
                      context.options.outLib,
                      context.options.outTop,
                      context.options.outDir);

  if (!context.options.outTest.empty()) {
    circuit->printRndVlogTest(*context.model,
                              context.options.outDir,
                              context.options.outTest,
                              10);
  }

  return true;
}

int hlsMain(HlsContext &context) {
  if (!parse(context))    { return -1; }
  if (!optimize(context)) { return -1; }
  if (!compile(context))  { return -1; }

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

    if (options.rtl.files().empty() && options.hls.files().empty()) {
      throw CLI::CallForAllHelp();
    }
  } catch(const CLI::ParseError &e) {
    return options.exit(e);
  }

  int result = 0;

  for (auto file : options.rtl.files()) {
    RtlContext context(file, options.rtl);
    result |= rtlMain(context);
  }

  for (auto file : options.hls.files()) {
    HlsContext context(file, options.hls);
    result |= hlsMain(context);
  }

  return result;
}
