//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/yosys_converter_model2.h"

#include "gate/model2/net.h"

int translateVerilogToModel2(const YosysToModel2Config &config) {
  YosysConverterModel2 translator(config);
  const auto netID = translator.getNetID();
  const auto &net = eda::gate::model::Net::get(netID);
  std::cout << net << std::endl;
  return 0;
}
