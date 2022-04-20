//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/model/gate.h"
#include "gate/model/netlist.h"
#include "hls/model/model.h"
#include "hls/model/printer.h"
#include "hls/parser/hil/parser.h"
#include "hls/scheduler/param_optimizer.h"
#include "hls/scheduler/latency_solver.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/parser.h"
#include "util/string.h"

#include "easylogging++.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <string>

INITIALIZE_EASYLOGGINGPP

using namespace eda::gate::model;
using namespace eda::rtl::compiler;
using namespace eda::rtl::library;
using namespace eda::rtl::model;
using namespace eda::utils;

int rtl_main(const std::string &filename) {
  LOG(INFO) << "Starting rtl_main " << filename;

  auto model = eda::rtl::parser::ril::parse(filename);
  if (model == nullptr) {
    std::cout << "Could not parse " << filename << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::cout << "------ p/v-nets ------" << std::endl;
  std::cout << *model << std::endl;

  Compiler compiler(FLibraryDefault::get());
  auto netlist = compiler.compile(*model);

  std::cout << "------ netlist ------" << std::endl;
  std::cout << *netlist;

  return 0;
}

int hls_main(const std::string &filename) {
  LOG(INFO) << "Starting hls_main " << filename;

  auto model = eda::hls::parser::hil::parse(filename);
  if (model == nullptr) {
    std::cout << "Could not parse " << filename << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::cout << *model;

  // Optimization criterion and constraints.
  eda::hls::scheduler::Criteria criteria(
    Throughput,
    Constraint(1000, 500000),                               // Frequency (kHz)
    Constraint(1000, 500000),                               // Throughput (=frequency)
    Constraint(0,    1000),                                  // Latency (cycles)
    Constraint(0,    std::numeric_limits<unsigned>::max()), // Power (does not matter)
    Constraint(1,    5000));                              // Area (number of LUTs)

  model->save();

  Indicators indicators;
  std::map<std::string, Parameters> params =
    eda::hls::scheduler::ParametersOptimizer::get().optimize(criteria, *model, indicators);

  model->save();

/*
  eda::hls::scheduler::LpSolver balancer;
  balancer.balance(*model);
  auto* balancedModel = balancer.getModel();
*/

  std::cout << "Balancing done.\n";
  std::cout << *model;

  std::ofstream output(filename + ".dot");
  printDot(output, *model);
  output.close();

  return 0;
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  std::cout << "Utopia EDA ";
  std::cout << VERSION_MAJOR << "." << VERSION_MINOR << " | ";
  std::cout << "Copyright (c) 2021 ISPRAS" << std::endl;

  if (argc <= 1) {
    std::cout << "Usage: " << argv[0] << " <input-file(s)>" << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  int result = 0;
  for (int i = 1; i < argc; i++) {
    const std::string filename = argv[i];

    int status = -1;
    if (ends_with(filename, ".ril")) {
      status = rtl_main(filename);
    } else if (ends_with(filename, ".hil")) {
      status = hls_main(filename);
    } else {
      std::cout << "Unknown format: " << filename << std::endl;
      status = -1;
    }

    result = (result == 0 ? status : result);
  }

  return result;
}

