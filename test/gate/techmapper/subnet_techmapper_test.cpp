//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/model/examples.h"

#include "gate/estimator/ppa_estimator.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/library/library.h"
#include "gate/premapper/cell_aigmapper.h"
#include "gate/techmapper/subnet_techmapper_base.h"
#include "gate/techmapper/matcher/pbool_matcher.h"
#include "gate/library/library.h"
#include "gate/optimizer/cut_extractor.h"

#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

#include <memory>

namespace eda::gate::techmapper {

using SubnetTechMapperSky130Test = SubnetTechMapperTest<sky130lib>;

TEST_F(SubnetTechMapperSky130Test, Consts) {
  auto builderPtr = std::make_shared<model::SubnetBuilder>();

  const auto idx0 = builderPtr->addCell(model::ONE);
  const auto idx1 = builderPtr->addCell(model::ZERO);

  builderPtr->addOutput(idx0);
  builderPtr->addOutput(idx1);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = this->commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, SimpleSubnet) {
  auto builderPtr = std::make_shared<model::SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx3 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx4 = builderPtr->addCell(model::AND, idx2, idx3);

  builderPtr->addOutput(idx4);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = this->commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, SimpleANDSubnet) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, idx1);

  builderPtr->addOutput(idx2);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, SimpleANDNOTSubnet) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, ~idx1);

  builderPtr->addOutput(idx2);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, FourInANDSubnet) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();
  const auto idx2 = builderPtr->addInput();
  const auto idx3 = builderPtr->addInput();

  const auto idx4 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx5 = builderPtr->addCell(model::AND, idx2, idx3);
  const auto idx6 = builderPtr->addCell(model::AND, idx4, idx5);

  builderPtr->addOutput(idx6);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, FourInANDNOTSubnet) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();
  const auto idx2 = builderPtr->addInput();
  const auto idx3 = builderPtr->addInput();

  const auto idx4 = builderPtr->addCell(model::AND, idx0, ~idx1);
  const auto idx5 = builderPtr->addCell(model::AND, idx2, ~idx3);
  const auto idx6 = builderPtr->addCell(model::AND, ~idx4, ~idx5);

  builderPtr->addOutput(idx6);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

/* Test for subnet for a sky130_ha cell
  0 <= in();
  1 <= in();
  4 <= and(0.0, 1.0);
  5 <= and(0.0, ~1.0);
  6 <= and(~0.0, 1.0);
  7 <= and(~5.0, ~6.0);
  2 <= out(4.0);
  3 <= out(~7.0);
 */
TEST_F(SubnetTechMapperSky130Test, HaCell) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx3 = builderPtr->addCell(model::AND, idx0, ~idx1);
  const auto idx4 = builderPtr->addCell(model::AND, ~idx0, idx1);
  const auto idx5 = builderPtr->addCell(model::AND, ~idx3, ~idx4);

  builderPtr->addOutput(idx2);
  builderPtr->addOutput(~idx5);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, RandomSubnet) {
  const auto subnetID = model::randomSubnet(6, 2, 20, 2, 2);
  const auto builderPtr = std::make_shared<SubnetBuilder>(subnetID);

  premapper::CellAigMapper aigMapper("aig");
  const auto premappedBuilder = aigMapper.map(builderPtr);
  const auto premappedSubnetID = premappedBuilder->make();
  std::cout << "Random Subnet:" << std::endl <<
          model::Subnet::get(subnetID) << std::endl;
  std::cout << "AIG-premapped Random Subnet:" << std::endl <<
          model::Subnet::get(premappedSubnetID) << std::endl;
  const auto mappedSubnetID =
    commonPart(premappedBuilder, 100000, 100000, 100000);
  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, GraphMLSubnetSmall) {
  auto builderPtr = parseGraphML("simple_spi_orig"); // 2k nodes
  // FIXME: do not call make(); implement another aigMapper.transform instead
  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID =
    commonPart(builderPtr, 1000000, 1000000, 1000000);
  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperSky130Test, DISABLED_GraphMLSubnetLarge) {
  auto builderPtr = parseGraphML("wb_conmax_orig"); // 80k nodes
  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID =
    commonPart(builderPtr, 10000000, 10000000, 10000000);
  checkEQ(subnetID, mappedSubnetID);
}

} // namespace eda::gate::techmapper
