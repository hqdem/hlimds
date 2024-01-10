//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
// #include "gate/techoptimizer/power_map/power_map.h"
// #include "gtest/gtest.h"
// 
// using namespace eda::gate::model;
// TEST(TechMapTest, switchFlow) {
//   if (!getenv("UTOPIA_HOME")) {
//     FAIL() << "UTOPIA_HOME is not set.";
//   }
//   std::cout << "test2" << std::endl;
//   using Link = Subnet::Link;
//   using LinkList = Subnet::LinkList;

//   SubnetBuilder builder;
//   LinkList links;
//   for (size_t i = 0; i < 3; i++) {
//     const auto idx = builder.addCell(eda::gate::model::IN, SubnetBuilder::INPUT);
//     links.emplace_back(idx);
//   } // 0 1 2
//   auto idx = builder.addCell(eda::gate::model::AND, links[0], links[1]);
//   links.emplace_back(idx); // 3

//   idx = builder.addCell(eda::gate::model::OR, links[0], links[2],links[3]);
//   links.emplace_back(idx); // 4

//   idx = builder.addCell(eda::gate::model::AND, links[0],links[3],links[4]);
//   links.emplace_back(idx); // 5

//   idx = builder.addCell(eda::gate::model::XOR,links[2], links[5],  SubnetBuilder::OUTPUT);
//   links.emplace_back(idx); //6

//   eda::gate::model::SubnetID subnetId = builder.make();
//   const auto &subnet = Subnet::get(subnetId);
//   std::cout << subnet << std::endl;

//   std::vector<double> cellActivities({0.1, 0.2, 0.3, 0.1, 0.2, 0.3, 0.25});
//   std::vector<double> computedSwitchFlow({0,0,0,0,0,0,0});

//   eda::gate::optimizer2::CutExtractor::Cut cut;

//   const auto cells = subnet.getEntries();
//   double r=0;
//   // for(int i=0; i < 7;i++){
//   //   r = eda::gate::tech_optimizer::PowerMap::switchFlow(i,cellActivities,computedSwitchFlow,subnet,cells);
//   // }
//   r = eda::gate::tech_optimizer::PowerMap::switchFlow(6,cut,cellActivities,cells);
//   std::cout <<r << std::endl;

//   EXPECT_DOUBLE_EQ(r,1.45);
//   };
