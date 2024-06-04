//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/printer/printer.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/parser/graphml_parser.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techmapper/library/liberty_manager.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/techmapper/utils/get_tech_attrs.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace eda::gate::techmapper {

using AigMapper  = premapper::AigMapper;
using CellSymbol = model::CellSymbol;
using Link       = model::Subnet::Link;
using LinkList   = model::Subnet::LinkList;
using NetBuilder = model::NetBuilder;
using Strategy   = Techmapper::Strategy;
using Subnet     = model::Subnet;
using SubnetBuilder    = model::SubnetBuilder;
using SubnetID   = model::SubnetID;

SDC sdc{100000000, 10000000000};

SubnetID parseGraphML(const std::string &fileName) {

  const path home = eda::env::getHomePath();
  const path dir = "test/data/gate/parser/graphml/OpenABC/graphml_openabcd";
  path name = path(fileName);
  name += ".bench.graphml";
  const path file = home / dir / name;
  if (!std::filesystem::exists(file)) {
    std::cout << "File " << file << " is missing!" << std::endl;
    assert(false);
  }
  parser::graphml::GraphMlParser parser;

  return parser.parse(file.string()).make();
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
    if (cell.getSymbol() != model::CellSymbol::UNDEF) {
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

SubnetID mapper(const Strategy strategy, const SubnetID subnetID) {

#ifdef UTOPIA_DEBUG
  std::cout << model::Subnet::get(subnetId) << std::endl;
#endif // UTOPIA_DEBUG

  Techmapper techmapper;
  techmapper.setStrategy(strategy);
  techmapper.setSDC(sdc);
  techmapper.setLibrary(techLib);

  AigMapper aigMapper("aig");
  const auto premappedSubnetID = aigMapper.transform(subnetID);

  SubnetBuilder builder;
  techmapper.techmap(premappedSubnetID, builder);
  SubnetID mappedSubnet = builder.make();

#ifdef UTOPIA_DEBUG
  std::cout << model::Subnet::get(mappedSubnet) << std::endl;
#endif // UTOPIA_DEBUG

  EXPECT_TRUE(checkAllCellsMapped(mappedSubnet));
  //checkEQ(subnetId, mappedSubnet);
  printVerilog(mappedSubnet);

  return mappedSubnet;
}

SubnetID simpleANDMapping(const Strategy strategy) {
  const auto primitiveANDSub  = createPrimitiveSubnet(CellSymbol::AND, 50, 15);
  std::cout << model::Subnet::get(primitiveANDSub) << std::endl;
  SubnetID mappedSubnet = mapper(strategy, primitiveANDSub);

  return mappedSubnet;
}

SubnetID simpleORMapping(const Strategy strategy) {
  const auto primitiveORSub  = createPrimitiveSubnet(CellSymbol::OR, 3, 13);
  std::cout << model::Subnet::get(primitiveORSub) << std::endl;
  SubnetID mappedSubnet = mapper(strategy, primitiveORSub);

  return mappedSubnet;
}

SubnetID graphMLMapping(const Strategy strategy,
                        const std::string &fileName) {
  SubnetID subnetId  = parseGraphML(fileName);
  SubnetID mappedSubnet = mapper(strategy, subnetId);

  return mappedSubnet;
}

SubnetID andNotMapping(const Strategy strategy) {
  SubnetBuilder builder;

  const auto idx0 = builder.addInput();
  const auto idx1 = builder.addInput();

  const auto idx2 = builder.addCell(model::AND, Link(idx0), Link(idx1));
  const auto idx3 = builder.addCell(model::AND, Link(idx0), Link(idx1));
  const auto idx4 = builder.addCell(model::AND, Link(idx2), Link(idx3));

  builder.addOutput(Link(idx4.idx, true));

  SubnetID subnetID = builder.make();
  SubnetID mappedSubnet = mapper(strategy, subnetID);

  return mappedSubnet;
}

SubnetID notNotAndMapping(const Strategy strategy) {
  SubnetBuilder builder;

  const auto idx0 = builder.addInput();
  const auto idx1 = builder.addInput();
  const auto idx2 = builder.addCell(
    model::AND, Link(idx0.idx, true), Link(idx1.idx, true));

  builder.addOutput(Link(idx2.idx, true));

  SubnetID subnetID = builder.make();
  SubnetID mappedSubnet = mapper(strategy, subnetID);

  return mappedSubnet;
}

SubnetID randomMapping(const Strategy strategy) {
  SubnetID randomSubnet = model::randomSubnet(6, 3, 50, 2, 6);
  SubnetID mappedSubnet = mapper(strategy, randomSubnet);

  return mappedSubnet;
}

NetID simpleNetMapping(const Strategy strategy) {
  NetBuilder netBuilder;
  std::vector<model::CellID> cells;

  for (size_t i = 0; i < 5; i++) {
    auto cellID = makeCell(model::CellSymbol::IN);
    cells.push_back(cellID);
    netBuilder.addCell(cellID);
  }

  auto cellIDAND0 = makeCell(model::CellSymbol::AND, cells[1], cells[2]);
  netBuilder.addCell(cellIDAND0);

  auto cellIDAND1 = makeCell(model::CellSymbol::AND, cells[3], cells[4]);
  netBuilder.addCell(cellIDAND1);

  auto cellIDDFF = makeCell(model::CellSymbol::DFF, cellIDAND0, cells[0]);
  netBuilder.addCell(cellIDDFF);

  auto cellIDAND2 = makeCell(model::CellSymbol::AND, cellIDDFF, cellIDAND1);
  netBuilder.addCell(cellIDAND2);

  auto cellOUT = makeCell(model::CellSymbol::OUT, cellIDAND2);
  netBuilder.addCell(cellOUT);

  Techmapper techmapper;
  techmapper.setStrategy(strategy);
  techmapper.setSDC(sdc);
  techmapper.setLibrary(techLib);

  const auto netID = netBuilder.make();

  // TODO: what about premapping in this case?
  //AigMapper aigMapper("aig");
  //const auto premappedNetID = aigMapper.transform(netID);

  //auto mappedNetID = techmapper.techmap(premappedNetID);
  auto mappedNetID = techmapper.techmap(netID);

  return mappedNetID;
}

SubnetID areaRecoveySubnetMapping(const Strategy strategy) {
  SubnetBuilder builder;

  const auto idxI1 = builder.addInput();
  const auto idxI2 = builder.addInput();
  const auto idxI3 = builder.addInput();
  const auto idxI4 = builder.addInput();
  const auto idxI5 = builder.addInput();
  const auto idxI6 = builder.addInput();

  const auto idxV1 = builder.addCell(model::AND, Link(idxI3), Link(idxI4));
  const auto idxV2 = builder.addCell(model::AND, Link(idxI5), Link(idxI6));
  const auto idxV3 = builder.addCell(model::AND, Link(idxI2), Link(idxV1));
  const auto idxV4 = builder.addCell(model::AND, Link(idxV1), Link(idxV2));
  const auto idxV5 = builder.addCell(model::AND, Link(idxI1), Link(idxV3));
  const auto idxV6 = builder.addCell(model::AND, Link(idxV3), Link(idxV5));

  builder.addOutput(idxV6);
  builder.addOutput(idxV4);

  SubnetID subnetID = builder.make();
  SubnetID mappedSubnet = mapper(strategy, subnetID);

  return mappedSubnet;
}

} // namespace eda::gate::techmapper
