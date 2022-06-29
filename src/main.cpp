//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "hls/compiler/compiler.h"
#include "hls/mapper/mapper.h"
#include "hls/model/model.h"
#include "hls/model/printer.h"
#include "hls/parser/hil/parser.h"
#include "hls/scheduler/latency_solver.h"
#include "hls/scheduler/param_optimizer.h"
#include "options.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/parser.h"
#include "util/string.h"

#include "easylogging++.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

INITIALIZE_EASYLOGGINGPP

int rtlMain(const std::string &file, const RtlOptions &options) {
  LOG(INFO) << "Starting rtlMain " << file;

  auto model = eda::rtl::parser::ril::parse(file);
  if (model == nullptr) {
    std::cout << "Could not parse " << file << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::cout << "------ p/v-nets ------" << std::endl;
  std::cout << *model << std::endl;

  eda::rtl::compiler::Compiler compiler(
      eda::rtl::library::FLibraryDefault::get());
  auto netlist = compiler.compile(*model);

  std::cout << "------ netlist ------" << std::endl;
  std::cout << *netlist;

  return 0;
}

int hlsMain(const std::string &file, const HlsOptions &options) {
  LOG(INFO) << "Starting hlsMain " << file;

  auto model = eda::hls::parser::hil::parse(file);
  if (model == nullptr) {
    std::cout << "Could not parse " << file << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::cout << *model;

  // Optimization criterion and constraints.
  eda::hls::model::Criteria criteria(
    PERF,
    eda::hls::model::Constraint<unsigned>(40000, 500000),    // Frequency (kHz)
    eda::hls::model::Constraint<unsigned>(1000,  500000),    // Performance (=frequency)
    eda::hls::model::Constraint<unsigned>(0,     1000),      // Latency (cycles)
    eda::hls::model::Constraint<unsigned>(),                 // Power (does not matter)
    eda::hls::model::Constraint<unsigned>(1,     10000000)); // Area (number of LUTs)

  model->save();

  // Map model nodes to meta elements.
  eda::hls::mapper::Mapper::get().map(
      *model, eda::hls::library::Library::get());

  eda::hls::model::Indicators indicators;
  eda::hls::scheduler::ParametersOptimizer::get().setBalancer(&eda::hls::scheduler::LatencyLpSolver::get());
  std::map<std::string, eda::hls::model::Parameters> params =
    eda::hls::scheduler::ParametersOptimizer::get().optimize(criteria,
                                                             *model,
                                                             indicators);

  model->save();

  std::cout << "Balancing done.\n";
  std::cout << *model;

  if (!options.outDot.empty()) {
    std::ofstream output(options.outDir + "/" + options.outDot);
    eda::hls::model::printDot(output, *model);
    output.close();
  }

  auto compiler = std::make_unique<eda::hls::compiler::Compiler>();
  auto circuit = compiler->constructCircuit(*model, "main");
  circuit->printFiles(options.outMlir,
                      options.outLib,
                      options.outTop,
                      options.outDir);

  if (!options.outTest.empty()) {
    circuit->printRndVlogTest(*model,
                              options.outDir,
                              options.outTest,
                              model->ind.ticks,
                              10);
  }

  eda::hls::library::Library::get().finalize();

  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  std::stringstream title;
  std::stringstream version;

  version << VERSION_MAJOR << "." << VERSION_MINOR;

  title << "Utopia EDA " << version.str() << " | ";
  title << "Copyright (c) 2021-2022 ISPRAS";

  Options options(title.str(), version.str());

  try {
    options.initialize("config.json", argc, argv);
  } catch(const CLI::ParseError &e) {
    return options.exit(e);
  }

  int result = 0;

  for (auto file : options.rtl.files()) {
    result |= rtlMain(file, options.rtl);
  }

  for (auto file : options.hls.files()) {
    result |= hlsMain(file, options.hls);
  }

  return result;
}
