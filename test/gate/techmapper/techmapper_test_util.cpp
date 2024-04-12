//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/sat_checker2.h"
#include "gate/model2/printer/printer.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/parser/gate_verilog.h"
#include "gate/parser/graphml_to_subnet.h"
#include "gate/parser/parser_test.h"
#include "gate/techmapper/library/liberty_manager.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/techmapper/utils/get_tech_attrs.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace eda::gate::techmapper {

using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

SDC sdc{100000000, 10000000000};

SubnetID parseGraphML(std::string fileName) {

  using path = std::filesystem::path;

  fileName += ".bench.graphml";

  const path dir = path("test") / "data" / "gate" / "parser"
                   / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = std::string(getenv("UTOPIA_HOME"));
  const path file = home / dir / fileName;

  eda::gate::parser::graphml::GraphMlSubnetParser parser;
  return parser.parse(file.string());
}

SubnetID createPrimitiveSubnet(CellSymbol symbol, size_t nIn, size_t arity) {
  Builder builder;
  LinkList links;

  for (size_t i = 0; i < nIn; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, arity);
  builder.addOutput(Subnet::Link(idx));

  return builder.make();
}

void printVerilog(SubnetID subnet) {
  eda::gate::model::ModelPrinter& verilogPrinter =
      eda::gate::model::ModelPrinter::getPrinter(eda::gate::model::ModelPrinter::VERILOG);
  std::ofstream outFile("test/data/gate/techmapper/print/techmappedNet.v");
  verilogPrinter.print(outFile,
                       model::Subnet::get(subnet),
                       "techmappedNet");
  std::cout << "Verilog-file: " <<
    "test/data/gate/techmapper/print/techmappedNet.v" << " is created." <<
    std::endl;
  outFile.close();
}

void printResults(SubnetID mappedSubnetId) {
  std::cout << "Area=" << getArea(mappedSubnetId) << std::endl;
  printStatistics(mappedSubnetId, techLib);
}

bool checkAllCellsMapped(SubnetID subnetID) {
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

void checkEQ(SubnetID origSubnetId, SubnetID mappedSubnetId) {
  Subnet &origSubnet = model::Subnet::get(origSubnetId);
  Subnet &mappedSubnet = model::Subnet::get(mappedSubnetId);
  std::unordered_map<size_t, size_t> map;

  for (int i = 0; i < origSubnet.getInNum(); i++) {
    map[i] = i;
  }
  for (int i = origSubnet.getOutNum(); i < 0; i--) {
    map[mappedSubnet.getEntries().size() - i] = origSubnet.getEntries().size() - i;
  }

  debugger2::SatChecker2& checker = debugger2::SatChecker2::get();

  EXPECT_TRUE(checker.equivalent(origSubnet, mappedSubnet, map).equal());
}

SubnetID mapper(Techmapper::MapperType mapperType, SubnetID subnetId) {
  //#ifdef UTOPIA_DEBUG
  std::cout << model::Subnet::get(subnetId) << std::endl;
  //#endif

  LibraryManager::get().loadLibrary(techLib);
  Techmapper techmapper(mapperType, sdc);
  SubnetID mappedSubnet = techmapper.techmap(subnetId);

  //#ifdef UTOPIA_DEBUG
  std::cout << model::Subnet::get(mappedSubnet) << std::endl;
  //#endif

  EXPECT_TRUE(checkAllCellsMapped(mappedSubnet));
  //checkEQ(subnetId, mappedSubnet);
  printVerilog(mappedSubnet);

  return mappedSubnet;
}

SubnetID simpleANDMapping(Techmapper::MapperType mapperType) {
  const auto primitiveANDSub  = createPrimitiveSubnet(CellSymbol::AND, 50, 15);
  std::cout << model::Subnet::get(primitiveANDSub) << std::endl;

  SubnetID mappedSubnet = mapper(mapperType, primitiveANDSub);

  return mappedSubnet;
}

SubnetID simpleORMapping(Techmapper::MapperType mapperType) {
  const auto primitiveORSub  = createPrimitiveSubnet(CellSymbol::OR, 3, 13);
  std::cout << model::Subnet::get(primitiveORSub) << std::endl;

  SubnetID mappedSubnet = mapper(mapperType, primitiveORSub);

  return mappedSubnet;
}

SubnetID graphMLMapping(Techmapper::MapperType mapperType, const std::string fileName) {
  SubnetID subnetId  = parseGraphML(fileName);

  SubnetID mappedSubnet = mapper(mapperType, subnetId);

  return mappedSubnet;
}

SubnetID andNotMapping(Techmapper::MapperType mapperType) {
  using Link = model::Subnet::Link;

  model::SubnetBuilder builder;

  const auto idx0 = builder.addInput();
  const auto idx1 = builder.addInput();

  const auto idx2 = builder.addCell(model::AND, Link(idx0), Link(idx1));
  const auto idx3 = builder.addCell(model::AND, Link(idx0), Link(idx1));
  const auto idx4 = builder.addCell(model::AND, Link(idx2), Link(idx3));

  builder.addOutput(Link(idx4.idx, true));

  SubnetID subnetID = builder.make();

  SubnetID mappedSubnet = mapper(mapperType, subnetID);

  return mappedSubnet;
}

SubnetID notNotAndMapping(Techmapper::MapperType mapperType) {
  using Link = model::Subnet::Link;

  model::SubnetBuilder builder;

  const auto idx0 = builder.addInput();
  const auto idx1 = builder.addInput();

  const auto idx2 = builder.addCell(model::AND, Link(idx0.idx, true), Link(idx1.idx, true));

  builder.addOutput(Link(idx2.idx, true));

  SubnetID subnetID = builder.make();

  SubnetID mappedSubnet = mapper(mapperType, subnetID);

  return mappedSubnet;
}

SubnetID randomMapping(Techmapper::MapperType mapperType) {
  SubnetID randomSubnet = model::randomSubnet(6, 3, 50, 2, 6);

  SubnetID mappedSubnet = mapper(mapperType, randomSubnet);

  return mappedSubnet;
}

NetID simpleNetMapping(Techmapper::MapperType mapperType) {
  model::NetBuilder netBuilder;
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

  Techmapper techmapper(techLib, mapperType, sdc);
  auto mappedNetID = techmapper.techmap(netBuilder.make());

  return mappedNetID;
}

SubnetID areaRecoveySubnetMapping(Techmapper::MapperType mapperType) {
  using Link = model::Subnet::Link;

  model::SubnetBuilder builder;

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

  SubnetID mappedSubnet = mapper(mapperType, subnetID);

  return mappedSubnet;
}

} // namespace eda::gate::techmapper
