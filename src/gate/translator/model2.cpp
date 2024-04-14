//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/yosys_converter_model2.h"

int translateVerilogToModel2(const YosysToModel2Config &config) {
  YosysConverterModel2 translator(config);
  return 0;
}
