//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>

struct TranslatorOptions;

/**
 * Translates a Verilog file to the gate-level Verilog file.
 */
namespace eda::gate::model {
int translateToGateVerilog(const std::string &file,
                           const TranslatorOptions &gateVerilog);
} // namespace eda::gate::model