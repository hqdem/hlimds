//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "gate/model/printer/net_printer.h"
#include "gate/techmapper/matcher/pbool_matcher.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/translator/graphml_test_utils.h"

#include <fstream>

namespace eda::gate::techmapper {

using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

std::shared_ptr<SubnetBuilder> parseGraphML(const std::string &fileName) {
  return translator::translateGmlOpenabc(fileName);
}

SubnetID createPrimitiveSubnet(const model::CellSymbol symbol,
                               const size_t nIn, const size_t arity) {
  SubnetBuilder builder;
  Subnet::LinkList links;

  for (size_t i = 0; i < nIn; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, arity);
  builder.addOutput(Subnet::Link(idx));

  return builder.make();
}

void printVerilog(const SubnetID subnet) {
  static constexpr auto name = "techmappedNet";
  std::ofstream outFile("test/data/gate/techmapper/print/techmappedNet.v");
  model::print(outFile, model::VERILOG, name, Subnet::get(subnet));
  std::cout << "Output Verilog file: " <<
    "test/data/gate/techmapper/print/techmappedNet.v" << std::endl;
  outFile.close();
}

bool checkAllCellsMapped(const SubnetID subnetID) {
  bool isTotalMapped = true;
  auto entr = Subnet::get(subnetID).getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entr); entryIndex++) {
    auto cell = entr[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isZero() || cell.isOne()) {
      continue;
    }
    if (cell.getSymbol() != model::CellSymbol::UNDEF) {
      isTotalMapped = false;
    }
    entryIndex += cell.more;
  }
  return isTotalMapped;
}

void checkEQ(const SubnetID origSubnetID, const SubnetID mappedSubnetID) {
  auto &checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(origSubnetID, mappedSubnetID).equal());
}

} // namespace eda::gate::techmapper
