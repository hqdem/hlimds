//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright <2023> ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <vector>

struct FirrtlConfig {
    bool debugMode;
    std::string topModule;
    std::string outputNamefile;
    std::vector<std::string> files;
};

/**
 * Translates the Verilog file to FIRRTL.
 */

int translateToFirrtl(const FirrtlConfig &config);
