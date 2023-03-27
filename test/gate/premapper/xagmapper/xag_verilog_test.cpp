//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include "gate/debugger/checker.h"
#include "gate/parser/gate_verilog_parser.h"
#include "gate/printer/dot.h"
#include "gate/premapper/xagmapper/xag_test.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <unordered_map>

using namespace eda::gate::premapper;
using namespace lorina;
using namespace eda::gate::model;

using GateBinding = std::unordered_map<Gate::Link, Gate::Link>;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using Link = Gate::Link;

TEST(XagPremapperVerilogTest, FirstTest) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  const std::filesystem::path subCatalog = "test/gate/premapper/xagmapper";
  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path prefixPath = homePath / subCatalog;
  const std::filesystem::path prefixPathIn = prefixPath / "input";
  const std::string filenames = prefixPath / "verilog_filenames.txt";

  std::ifstream in(filenames);
  std::string infile;

  while (std::getline(in, infile)) {
    std::string filename = prefixPathIn / (infile + ".v");

    text_diagnostics consumer;
    diagnostic_engine diag(&consumer);

    GateVerilogParser parser(infile);

    return_code result = read_verilog(filename, parser, &diag);
    EXPECT_EQ(result, return_code::success);

//    std::shared_ptr<GNet> net = std::make_shared<GNet>(*reader.getGnet());
//    net->sortTopologically();
//    GateIdMap gmap;

    dump(*parser.getGnet());
//    std::shared_ptr<GNet> premapped = premap(net, gmap);
//    std::cout << checkEquivalence(net, premapped, gmap) << std::endl;
    delete parser.getGnet();
  }
  in.close();
}
