
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include <filesystem>
#include <cassert>

#include "gate/model/examples.h"

#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/techoptimizer/techoptimizer.h"
#include "gate/model2/utils/subnet_random.h"

#include "gtest/gtest.h"
#include "gate/model2/object.h"
#include "gate/model2/printer/printer.h"
#include "gate/debugger2/sat_checker2.h"

#include "gate/parser/graphml_to_subnet.h"
#include "gate/techoptimizer/util/get_tech_attr.h"

using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

const /*std::filesystem::path*/ std::string libertyPath = std::string(getenv("UTOPIA_HOME")) + "/test/data/gate/tech_mapper";

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
  std::ofstream outFile("test/data/gate/tech_mapper/print/techmappedNet.v");
  verilogPrinter.print(outFile,
                       model::Subnet::get(subnet),
                       "techmappedNet");
  outFile.close();
}

bool checkAllCellsMapped(SubnetID subnetID) {
  bool isTotalMapped = true;
  auto entr = model::Subnet::get(subnetID).getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entr); entryIndex++) {
    auto cell = entr[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isZero() || cell.isOne()) {
      continue;
    }
    if (cell.getSymbol() != model::CellSymbol::CELL) {
      isTotalMapped = false;
    }
    entryIndex += cell.more;
  }
  return isTotalMapped;
}
/*
TEST(TechMapTest, RandomSubnet) {
  SubnetID randomSubnet = model::randomSubnet(6, 3, 50, 2, 6);
  std::cout << model::Subnet::get(randomSubnet) << std::endl;

  Techmapper techmapper;
  techmapper.setLiberty(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

  SubnetID mappedSub = techmapper.techmap(randomSubnet);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
Subnet &mappedSubnet = Subnet::get(mappedSub);
eda::gate::model::ModelPrinter& verilogPrinter =
    eda::gate::model::ModelPrinter::getPrinter(eda::gate::model::ModelPrinter::VERILOG);
std::ofstream outFile("test/data/gate/tech_mapper/print/techmappedNet.v");
verilogPrinter.print(outFile,
    model::Subnet::get(mappedSub),
"techmappedNet");
outFile.close();
}*/

TEST(TechMapTest, graphML) {

SubnetID subnetId  = parseGraphML("aes_orig");

Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                      Techmapper::MapperType::SIMPLE_AREA_FUNC);

/*auto entries = model::Subnet::get(subnetId).getEntries();

std::cout <<  entries[17492].cell.getType().getName() << std::endl;*/
//std::cout <<  entries[17681].cell.link[0].idx << " " << entries[17681].cell.link[1].idx<< std::endl;
SubnetID mappedSub = techmapper.techmap(subnetId);

//std::cout << model::Subnet::get(mappedSub) << std::endl;
printVerilog(mappedSub);

EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleANDSubnet) {
  const auto primitiveANDSub  = createPrimitiveSubnet(CellSymbol::AND, 13, 2);
  std::cout << model::Subnet::get(primitiveANDSub) << std::endl;

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                        Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(primitiveANDSub);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  SubnetID mappedSub2 = techmapper.techmap(primitiveANDSub);

  std::cout << model::Subnet::get(mappedSub2) << std::endl;

  printVerilog(mappedSub);

  std::cout << getArea(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleORSubnet) {
  const auto primitiveORSub  = createPrimitiveSubnet(CellSymbol::OR, 3, 13);

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                      Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(primitiveORSub);

  auto entries = model::Subnet::get(mappedSub).getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;
    std::cout << cell.getSymbol() << std::endl;
    entryIndex += cell.more;
  }
  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);

  std::cout << getArea(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleSub) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  using Link = model::Subnet::Link;
  using LinkList = model::Subnet::LinkList;

  model::SubnetBuilder builder;
  LinkList links;
  LinkList links1;
  LinkList links2;

  for (size_t i = 0; i < 2; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }
  for (size_t i = 0; i < 2; i++) {
    const auto idx = builder.addInput();
    links1.emplace_back(idx);
  }

  const auto idx1 = builder.addCell(model::AND, links);
  links2.emplace_back(idx1);

  const auto idx2 = builder.addCell(model::AND, links1);
  links2.emplace_back(idx2);

  const auto idx3 = builder.addCell(model::AND, links2);
  const auto idxOUT = builder.addOutput(Link(idx3));

  SubnetID subnetID = builder.make();

  //SubnetID subnetID = model::randomSubnet(5, 1, 7, 1, 2);
  auto &subnet = model::Subnet::get(subnetID);
  std::cout << subnet << std::endl;

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_.lib",
                      Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(subnetID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
  printVerilog(mappedSub);

  Subnet &mappedSubnet = model::Subnet::get(mappedSub);
  std::unordered_map<size_t, size_t> map;

  map[0] = 0;
  map[1] = 1;
  map[2] = 2;
  map[3] = 3;
  map[idxOUT.idx] = 5;

  debugger2::SatChecker2& checker = debugger2::SatChecker2::get();
  EXPECT_TRUE(checker.equivalent(subnet, mappedSubnet, map).equal());
}

TEST(TechMapTest, ANDNOTNOTAND) {
  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  using Link = model::Subnet::Link;
  using LinkList = model::Subnet::LinkList;

  model::SubnetBuilder builder;
  LinkList links;
  LinkList links2;

  const auto idx0 = builder.addInput();
  links.emplace_back(idx0);
  const auto idx1 = builder.addInput();
  links.emplace_back(idx1);

  const auto idx2 = builder.addCell(model::AND, Link(idx0), Link(idx1));

  const auto idx3 = builder.addCell(model::AND, Link(idx0), Link(idx1));

  const auto idx4 = builder.addCell(model::AND, Link(idx2), Link(idx3));

  builder.addOutput(idx4);

  SubnetID subnetID = builder.make();

  const auto &subnet = model::Subnet::get(subnetID);
  std::cout << subnet << std::endl;

    Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                      Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(subnetID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);
  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, DFFMapping) {
  auto cellID = makeCell(model::CellSymbol::DFF);

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                        Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(cellID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}
TEST(TechMapTest, DFFrsMapping) {
  auto cellID = makeCell(model::CellSymbol::DFFrs);

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                        Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(cellID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}
TEST(TechMapTest, LatchMapping) {
  auto cellID = makeCell(model::CellSymbol::LATCH);

  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                        Techmapper::MapperType::SIMPLE_AREA_FUNC);

  SubnetID mappedSub = techmapper.techmap(cellID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}
}
