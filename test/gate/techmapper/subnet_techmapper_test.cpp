//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "context/utopia_context.h"
#include "gate/estimator/ppa_estimator.h"
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

class SubnetTechMapperTest : public testing::Test {
protected:
  SubnetTechMapperTest() {
    auto &library = context_.techMapContext.library;
    library = std::make_unique<library::SCLibrary>(techLib);
    
    if (library != nullptr) {
      std::cout << "Loaded Liberty file: " << techLib << std::endl;
    } else {
      throw std::runtime_error("File loading failed");
    }
    pBoolMatcher_ = std::move(
        Matcher<PBoolMatcher, kitty::dynamic_truth_table>::create(
          library->getCombCells()));

  }
  ~SubnetTechMapperTest() override {}
  void SetUp() override {}
  void TearDown() override {}

  const model::SubnetID commonPart(
      const std::shared_ptr<model::SubnetBuilder> builderPtr,
      const float maxArea,
      const float maxDelay,
      const float maxPower) {
    criterion::Objective objective(criterion::AREA);
    criterion::Constraints constraints = {
      criterion::Constraint(criterion::AREA, maxArea),
      criterion::Constraint(criterion::DELAY, maxDelay),
      criterion::Constraint(criterion::POWER, maxPower)
    };

    context_.criterion =
      std::make_unique<criterion::Criterion>(objective, constraints);
    std::unique_ptr<optimizer::CutExtractor> cutExtractor{};
    auto cutProvider = [&](const model::SubnetBuilder &builder,
                           const size_t entryID) {
      cutExtractor = 
        std::make_unique<optimizer::CutExtractor>(&builder, 6, true);
      return cutExtractor->getCuts(entryID);
      };

    auto matchFinder = [&](const model::SubnetBuilder &builder,
                           const optimizer::Cut &cut) {
                            return pBoolMatcher_->match(builder, cut);};

    std::unique_ptr<SubnetTechMapperBase> techmapper =
      std::make_unique<SubnetTechMapperBase>(
        name_, context_, cutProvider, matchFinder, estimator::getPPA);

    auto builder = techmapper->map(builderPtr);

    EXPECT_TRUE(builder != nullptr);
    techmapper.reset(nullptr);

    if (builder != nullptr) {
      const auto mappedSubnetID = builder->make();
      EXPECT_TRUE(checkAllCellsMapped(mappedSubnetID));

      printStatistics(mappedSubnetID, *context_.techMapContext.library);
      printVerilog(mappedSubnetID);
      std::cout << "Mapped Subnet: " << model::Subnet::get(mappedSubnetID);
      return mappedSubnetID;
    } else {
      return builderPtr->make(); // TODO
    }
  }

  std::string name_ = "UtopiaTechMapper";
  optimizer::CutExtractor *cutExtractor_ = nullptr;
  std::unique_ptr<PBoolMatcher> pBoolMatcher_ {};
  context::UtopiaContext context_;

};

TEST_F(SubnetTechMapperTest, SimpleSubnet) {
  auto builderPtr = std::make_shared<model::SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx3 = builderPtr->addCell(model::AND, idx0, idx1);
  const auto idx4 = builderPtr->addCell(model::AND, idx2, idx3);

  builderPtr->addOutput(idx4);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperTest, SimpleANDSubnet) {
  auto builderPtr = std::make_shared<SubnetBuilder>();

  const auto idx0 = builderPtr->addInput();
  const auto idx1 = builderPtr->addInput();

  const auto idx2 = builderPtr->addCell(model::AND, idx0, idx1);

  builderPtr->addOutput(idx2);

  const auto subnetID = builderPtr->make();
  const auto mappedSubnetID = commonPart(builderPtr, 1000, 1000, 1000);

  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperTest, FourInANDSubnet) {
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

TEST_F(SubnetTechMapperTest, DISABLED_RandomSubnet) {
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

TEST_F(SubnetTechMapperTest, GraphMLSubnetSmall) {
  auto builderPtr = parseGraphML("simple_spi_orig"); // 2k nodes
  premapper::CellAigMapper aigMapper("aig");
  // TODO: it's better to use buffer-insert pass
  // FIXME: do not call make(); implement another aigMapper.transform instead
  const auto subnetID = builderPtr->make();
  const auto premappedBuilder = aigMapper.map(builderPtr);
  const auto mappedSubnetID =
    commonPart(premappedBuilder, 1000000, 1000000, 1000000);
  checkEQ(subnetID, mappedSubnetID);
}

TEST_F(SubnetTechMapperTest, DISABLED_GraphMLSubnetLarge) {
  auto builderPtr = parseGraphML("wb_conmax_orig"); // 80k nodes
  premapper::CellAigMapper aigMapper("aig");
  const auto subnetID = builderPtr->make();
  const auto premappedBuilder = aigMapper.map(builderPtr); // TODO: it's better to use buffer-insert pass
  const auto mappedSubnetID =
    commonPart(premappedBuilder, 10000000, 10000000, 10000000);
  checkEQ(subnetID, mappedSubnetID);
}

} // namespace eda::gate::techmapper
