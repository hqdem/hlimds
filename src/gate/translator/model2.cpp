//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/net.h"
#include "gate/translator/yosys_converter_model2.h"

int translateVerilogToModel2(const YosysToModel2Config &config) {
  YosysConverterModel2 translator(config);
#ifdef UTOPIA_DEBUG
  const auto netID = translator.getNetID();
  const auto &net = eda::gate::model::Net::get(netID);
  std::cout << net << std::endl;
#endif // UTOPIA_DEBUG
  return 0;
}
