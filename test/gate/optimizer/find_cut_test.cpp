//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/util.h"
#include "gate/parser/gate_verilog.h"
#include "gtest/gtest.h"
#include "gate/printer/dot.h"

#include <filesystem>
#include <string>

using namespace eda::gate::parser::verilog;

namespace eda::gate::optimizer {

  TEST(FindCutTest, FindCut_c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path prefixPath = homePath / "build" / subCatalog;
    const std::string infile = "c17.v";

    std::string filename = prefixPath / infile;
    lorina::text_diagnostics consumer;
    lorina::diagnostic_engine diag(&consumer);

    GateVerilogParser parser(infile);
    assert(lorina::return_code::success ==
           read_verilog(filename, parser, &diag));

    CutStorage storage = findCuts(parser.getGnet(), 4, 100);

    for (auto &[v, cs]: storage.cuts) {
      for (const auto &c: cs) {
        GateId failed;

        if (!isCut(v, c, failed)) {
          FAIL() << "Wrong cut for v " << v << "; failed " << failed << std::endl;
        }
      }
    }

    delete parser.getGnet();
  }

} // namespace eda::gate::optimizer
