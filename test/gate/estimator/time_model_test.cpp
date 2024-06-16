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

void commonPart(std::string &libName, std::string &cellTypeName,
  const float inputTransTime, const float totalOutputCap,
  const float slewRef, const float delayRef) {

  const path homePath = eda::env::getHomePath();
  const path filePath = homePath / libName;

  TokenParser tokParser;
  FILE *file = fopen(filePath.generic_string().c_str(), "rb");
  Group *ast = tokParser.parseLibrary(file,
              filePath.generic_string().c_str());
  Library lib;
  AstParser parser(lib, tokParser);
  parser.run(*ast);
  fclose(file);

  int timingSense = 0;
  float slew = 0, delay = 0, cap = 0;
  NLDM::delayEstimation(lib, cellTypeName,
                        inputTransTime,
                        totalOutputCap,
                        timingSense, slew, delay, cap);
  EXPECT_DOUBLE_EQ(slew, slewRef);
  EXPECT_DOUBLE_EQ(delay, delayRef);
}

std::string libName = "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

TEST(Estimators, a2111o4) {
  std::string cellName = "sky130_fd_sc_hd__a2111o_4";
  commonPart(libName, cellName, 0.053133, 0.191204, 0.37748932838439941, 0.4451143741607666);
}

TEST(Estimators, o21a4) {
  std::string cellName = "sky130_fd_sc_hd__o21a_4";
  commonPart(libName, cellName, 0.053133, 0.001627, 0.024740446358919144, 0.09489276260137558);
}

TEST(Estimators, a211o2) {
  std::string cellName = "sky130_fd_sc_hd__a211o_2";
  commonPart(libName, cellName, 0.099999, 0.002468, 0.034578997641801834, 0.11193791031837463);
}

} // eda::gate::techmapper
