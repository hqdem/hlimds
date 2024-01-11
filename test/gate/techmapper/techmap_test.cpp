
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

using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

const std::filesystem::path homePathTechMap = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path libertyDirrectTechMap = homePathTechMap / "test" / "data" / "gate" / "tech_mapper";

SubnetID createPrimitiveSubnet(CellSymbol symbol, size_t nIn, size_t arity) {
  Builder builder;
  LinkList links;

  for (size_t i = 0; i < nIn; i++) {
    const auto idx = builder.addCell(CellSymbol::IN, Builder::INPUT);
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, arity);
  builder.addCell(CellSymbol::OUT, Link(idx), Builder::OUTPUT);

  return builder.make();
}

bool checkAllCellsMapped(SubnetID subnetID) {
  bool isTotalMapped = true;
  const auto &subnet = model::Subnet::get(subnetID);

  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isZero() || cell.isOne()) {
      continue;
    }
    if (model::CellType::get(cell.type).getSymbol() != model::CellSymbol::CELL ) {
      isTotalMapped = false;

      std::cout << model::CellType::get(cell.type).getSymbol() << "  " << entryIndex << std::endl;
    }
    entryIndex += cell.more;
  }
  std::cout << model::CellSymbol::CELL << std::endl;
  return isTotalMapped;
}

TEST(TechMapTest, RandomSubnet) {
  SubnetID randomSubnet = model::randomSubnet(50, 13, 1000, 1, 2);

  Techmaper techmaper;
  techmaper.setLiberty(libertyDirrectTechMap.string() +
      "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
  techmaper.setMapper(Techmaper::TechmaperType::FUNC);
  techmaper.setStrategy(Techmaper::TechmaperStrategyType::SIMPLE);

  SubnetID mappedSub = techmaper.techmap(randomSubnet);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleANDSubnet) {
  const auto primitiveANDSub  = createPrimitiveSubnet(CellSymbol::AND, 1000, 2);
  std::cout << model::Subnet::get(primitiveANDSub) << std::endl;

  Techmaper techmaper;
  techmaper.setLiberty(libertyDirrectTechMap.string() +
                       "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
  techmaper.setMapper(Techmaper::TechmaperType::FUNC);
  techmaper.setStrategy(Techmaper::TechmaperStrategyType::SIMPLE);

  SubnetID mappedSub = techmaper.techmap(primitiveANDSub);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}

TEST(TechMapTest, SimpleORSubnet) {
  const auto primitiveORSub  = createPrimitiveSubnet(CellSymbol::OR, 13, 13);

  Techmaper techmaper;

  techmaper.setLiberty(libertyDirrectTechMap.string() +
                       "/simple_liberty.lib");
  techmaper.setMapper(Techmaper::TechmaperType::FUNC);
  techmaper.setStrategy(Techmaper::TechmaperStrategyType::SIMPLE);

  SubnetID mappedSub = techmaper.techmap(primitiveORSub);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

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
    const auto idx = builder.addCell(model::IN, model::SubnetBuilder::INPUT);
    links.emplace_back(idx);
  }

  const auto idx1 = builder.addCell(model::AND, links);
  links2.emplace_back(idx1);

  links.clear();
  for (size_t i = 0; i < 2; i++) {
    const auto idx = builder.addCell(model::IN, model::SubnetBuilder::INPUT);
    links.emplace_back(idx);
  }
  const auto idx2 = builder.addCell(model::AND, links);
  links2.emplace_back(idx2);

  const auto idx3 = builder.addCell(model::AND, links2);
  builder.addCell(model::OUT, Link(idx3), model::SubnetBuilder::OUTPUT);

  SubnetID subnetID = builder.make();

  //SubnetID subnetID = model::randomSubnet(5, 1, 7, 1, 2);
  const auto &subnet = model::Subnet::get(subnetID);
  std::cout << subnet << std::endl;
  Techmaper techmaper;

  techmaper.setLiberty(libertyDirrectTechMap.string() +
                       "/sky130_fd_sc_hd__ff_n40C_1v95.lib");
  techmaper.setMapper(Techmaper::TechmaperType::FUNC);
  techmaper.setStrategy(Techmaper::TechmaperStrategyType::SIMPLE);

  SubnetID mappedSub = techmaper.techmap(subnetID);

  std::cout << model::Subnet::get(mappedSub) << std::endl;

  EXPECT_TRUE(checkAllCellsMapped(mappedSub));
}
}