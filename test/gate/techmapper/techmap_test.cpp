
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"

#include "gate/parser/gate_verilog.h"
#include "gate/parser/parser_test.h"
#include "gate/techoptimizer/techmapper.h"
#include "gate/model2/utils/subnet_random.h"

#include "gtest/gtest.h"
#include "gate/model2/object.h"
#include "gate/model2/printer/printer.h"
#include "gate/debugger2/sat_checker2.h"

using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

const /*std::filesystem::path*/ std::string libertyPath = std::string(getenv("UTOPIA_HOME")) + "/test/data/gate/tech_mapper";

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

void printVerilog(const SubnetID subnetID) {
  const auto &subnet = model::Subnet::get(subnetID);
  auto entries = subnet.getEntries();

  model::NetBuilder netBuilder;

  model::CellID cellIDArray[std::size(entries)];

  for (size_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto subnetCell = entries[entryIndex].cell;

    //model::CellID cellID;

    if (subnetCell.isIn()) {
      auto cellID = makeCell(model::IN);
      cellIDArray[entryIndex] = cellID;
      netBuilder.addCell(cellID);
    } else if (subnetCell.isOut()) {
      //cellID = makeCell(model::CellSymbol::OUT, cellIDArray[cell.link[0].idx]);
    } else {
      model::Cell::LinkList linkList;

      for (const auto &link : subnetCell.link) {
        linkList.emplace_back(cellIDArray[link.idx]);
      }
      if (subnetCell.more > 0) {
        for (int i = 1; i <= subnetCell.more; i++) {
          for (const auto &link :entries[entryIndex + i].link) {
            linkList.emplace_back(cellIDArray[link.idx]);
          }
        }
      }
      //cellID = makeCell(cell.getTypeID(), linkList);
      linkList.clear();
    }
    //cellIDArray[entryIndex] = cellID;
//    netBuilder.addCell(cellID);

    entryIndex += subnetCell.more;
  }

  // Create an instance of the NetPrinter class for the VERILOG format
  eda::gate::model::NetPrinter& verilogPrinter =
      eda::gate::model::NetPrinter::getPrinter(eda::gate::model::VERILOG);

  // Open a stream for writing Verilog code to a file or console
  std::ofstream outFile("test/data/gate/tech_mapper/print/techmappedNet.v");  // Or use std::cout to print to the console

  // Call the NetPrinter::print method to generate Verilog code
  verilogPrinter.print(outFile,
                       model::Net::get(netBuilder.make()),
                       "techmappedNet");

  // Close the stream
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

TEST(TechMapTest, RandomSubnet) {
  SubnetID randomSubnet = model::randomSubnet(6, 3, 50, 2, 6);
  std::cout << model::Subnet::get(randomSubnet) << std::endl;

  Techmapper techmapper;
  techmapper.setLiberty(libertyPath + "/simple_liberty.lib");
  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

  SubnetID mappedSub = techmapper.techmap(randomSubnet);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleANDSubnet) {
  const auto primitiveANDSub  = createPrimitiveSubnet(CellSymbol::AND, 13, 2);
  std::cout << model::Subnet::get(primitiveANDSub) << std::endl;

  Techmapper techmapper;
  techmapper.setLiberty(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

  SubnetID mappedSub = techmapper.techmap(primitiveANDSub);

  std::cout << model::Subnet::get(mappedSub) << std::endl;
  printVerilog(mappedSub);

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleORSubnet) {
  const auto primitiveORSub  = createPrimitiveSubnet(CellSymbol::OR, 3, 13);

  Techmapper techmapper;

  techmapper.setLiberty(libertyPath + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

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
  LinkList links2;

  for (size_t i = 0; i < 2; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx1 = builder.addCell(model::AND, links);
  links2.emplace_back(idx1);

  links.clear();
  for (size_t i = 0; i < 2; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }
  const auto idx2 = builder.addCell(model::AND, links);
  links2.emplace_back(idx2);

  const auto idx3 = builder.addCell(model::AND, links2);
  const auto idxOUT = builder.addOutput(Link(idx3));

  SubnetID subnetID = builder.make();

  //SubnetID subnetID = model::randomSubnet(5, 1, 7, 1, 2);
  auto &subnet = model::Subnet::get(subnetID);
  std::cout << subnet << std::endl;
  Techmapper techmapper;

  techmapper.setLiberty(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");

  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

  SubnetID mappedSub = techmapper.techmap(subnetID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));

  Subnet &mappedSubnet = model::Subnet::get(mappedSub);
  std::unordered_map<size_t, size_t> map;

  map[0] = 0;
  map[1] = 1;
  map[3] = 2;
  map[4] = 3;
  map[idxOUT] = 5;

  debugger2::SatChecker2& checker = debugger2::SatChecker2::get();
  //EXPECT_TRUE(checker.equivalent(subnet, mappedSubnet, map).equal());
  printVerilog(mappedSub);
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

  const auto idx2 = builder.addCell(model::AND, Link(idx0, true), Link(idx1));

  const auto idx3 = builder.addCell(model::AND, Link(idx0), Link(idx1, true));

  const auto idx4 = builder.addCell(model::AND, Link(idx2), Link(idx3));

  builder.addOutput(idx4);

  SubnetID subnetID = builder.make();

  const auto &subnet = model::Subnet::get(subnetID);
  std::cout << subnet << std::endl;
  Techmapper techmapper;

  techmapper.setLiberty(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
  techmapper.setMapper(Techmapper::TechmapperType::FUNC);
  techmapper.setStrategy(Techmapper::TechmapperStrategyType::SIMPLE);

  SubnetID mappedSub = techmapper.techmap(subnetID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}
}
