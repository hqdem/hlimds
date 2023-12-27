//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright <2023> ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

struct FirRtlOptions;

/**
 * Translates the Verilog file to FIRRTL, the result is written
 * to the file named after the input file with the '.fir' suffix.
 */
int translateToFirrtl(const std::string &file, const FirRtlOptions &options);
