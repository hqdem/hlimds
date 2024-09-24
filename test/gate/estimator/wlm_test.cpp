//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simple_time_model.h"
#include "util/env.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>

namespace eda::gate::estimator {

using path = std::filesystem::path;
#if 0 //TODO FIXME
TEST(Estimators, wlmTest) {
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

  WLM wlm;
  size_t fanoutCount = 1;
  bool f1 = (wlm.getLength(fanoutCount) == 23.274599075317383);
  bool f2 = (wlm.getFanoutCap(fanoutCount) == 0.0046549197286367416);

  EXPECT_TRUE((f1 == 1) && (f2 == 1));

  std::cout << "Length\tCap\tRes\n";
  for (size_t i = 1; i < 6; ++i)
    std::cout << wlm.getLength(i)
              << '\t'
              << wlm.getFanoutCap(i)
              << '\t'
              << wlm.getFanoutRes(i)
              << std::endl;
}
#endif
} // namespace eda::gate::estimator
