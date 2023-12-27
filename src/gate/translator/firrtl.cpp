//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright <2023> ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/yosys_converter_firrtl.h"
#include "options.h"

int translateToFirrtl(
    const std::string &file,
    const FirRtlOptions &options) {
  YosysConverterFirrtl(file, options.top);
  return 0;
}
