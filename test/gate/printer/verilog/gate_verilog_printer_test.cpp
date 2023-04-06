//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/printer/verilog/gate_verilog_printer.h"


#include "gtest/gtest.h"

namespace eda::gate::printer {

void printerTest() {
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;

  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeOr(10, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
  GateVerilogPrinter::get().print(std::cout, *net);
}

TEST(GateVerilogPrinter, SimpleTest) {
  printerTest();
}

} // namespace eda::gate::printer
