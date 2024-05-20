//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/cut_based/delay_estmt/delay_estmt.h"
#include "util/env.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>

namespace eda::gate::techmapper {

using path = std::filesystem::path;

TEST(DelayEstmt, wlmTest) {
  std::string file_name = "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

  const path homePath = eda::env::getHomePath();
  const path filePath = homePath / file_name;

  TokenParser tokParser;
  FILE *file = fopen(filePath.generic_string().c_str(), "rb");
  Group *ast = tokParser.parseLibrary(file,
              filePath.generic_string().c_str());
  Library lib;
  AstParser parser(lib, tokParser);
  parser.run(*ast);
  fclose(file);

  DelayEstimator d1(lib);
  size_t fanout_count = 1;
  bool f1 = (d1.wlm.getLength(fanout_count) == 23.274599075317383);
  bool f2 = (d1.wlm.getFanoutCap(fanout_count) == 0.0046549197286367416);

  EXPECT_TRUE((f1 == 1) && (f2 == 1));

  std::cout << "Length\tCap\tRes\n";
  for (size_t i = 1; i < 6; ++i)
    std::cout << d1.wlm.getLength(i)
              << '\t'
              << d1.wlm.getFanoutCap(i)
              << '\t'
              << d1.wlm.getFanoutRes(i)
              << std::endl;
}

} // namespace eda::gate::techmapper
