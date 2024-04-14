//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

struct YosysToModel2Config {
  bool debugMode;
  std::string topModule;
  std::vector<std::string> files;
};

/**
 * Translates the Verilog file to model2.
 */

int translateVerilogToModel2(const YosysToModel2Config &config);
