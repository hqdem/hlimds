//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/library/sdc_manager.h"
#include "gate/model/printer/printer.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/premapper/cell_aigmapper.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

#include <fstream>

namespace eda::gate::techmapper {

using AigMapper  = premapper::CellAigMapper;
using CellSymbol = model::CellSymbol;
using Link       = model::Subnet::Link;
using LinkList   = model::Subnet::LinkList;
using NetBuilder = model::NetBuilder;
using SDC        = library::SDC;
using Subnet     = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetBuilderPtr = optimizer::SubnetBuilderPtr;
using SubnetID   = model::SubnetID;

std::shared_ptr<SubnetBuilder> parseGraphML(const std::string &fileName) {
  return translator::translateGmlOpenabc(fileName);
}

SubnetID createPrimitiveSubnet(const CellSymbol symbol,
                              const size_t nIn, const size_t arity) {
  SubnetBuilder builder;
  LinkList links;

  for (size_t i = 0; i < nIn; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, arity);
  builder.addOutput(Subnet::Link(idx));

  return builder.make();
}

void printVerilog(const SubnetID subnet) {
  using ModelPrinter = eda::gate::model::ModelPrinter;
  ModelPrinter& verilogPrinter =
    ModelPrinter::getPrinter(ModelPrinter::VERILOG);
  std::ofstream outFile("test/data/gate/techmapper/print/techmappedNet.v");
  verilogPrinter.print(outFile, model::Subnet::get(subnet), "techmappedNet");
  std::cout << "Output Verilog file: " <<
    "test/data/gate/techmapper/print/techmappedNet.v" << std::endl;
  outFile.close();
}

bool checkAllCellsMapped(const SubnetID subnetID) {
  bool isTotalMapped = true;
  auto entr = model::Subnet::get(subnetID).getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entr); entryIndex++) {
    auto cell = entr[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isZero() || cell.isOne()) {
      continue;
    }
    if (cell.getSymbol() != CellSymbol::UNDEF) {
      isTotalMapped = false;
    }
    entryIndex += cell.more;
  }
  return isTotalMapped;
}

void checkEQ(const SubnetID origSubnetID, const SubnetID mappedSubnetID) {
  debugger::SatChecker& checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(origSubnetID, mappedSubnetID).equal());
}

} // namespace eda::gate::techmapper
