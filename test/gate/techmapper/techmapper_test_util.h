//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/ppa_estimator.h"
#include "gate/library/library_factory.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "gate/techmapper/subnet_techmapper_pcut.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <cassert>

using path = std::filesystem::path;

namespace eda::gate::techmapper {

using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

inline const path home = eda::env::getHomePath();
inline const path techLib = home /
  "test/data/gate/techmapper" /
  "sky130_fd_sc_hd__ff_100C_1v65.lib"; // TODO

inline const path sdcPath = home /
  "test/data/gate/techmapper" /
  "test.sdc"; // TODO

inline const path sky130lib = home / "test/data/gate/techmapper/"
  "sky130_fd_sc_hd__ff_100C_1v65.lib";

std::shared_ptr<SubnetBuilder> parseGraphML(const std::string &fileName);
void printVerilog(const SubnetID subnet);
bool checkAllCellsMapped(const SubnetID subnetID);
void checkEQ(const SubnetID origSubnetID, const SubnetID mappedSubnetID);

template <const path &LIBPATH>
class LibraryInitializer : public testing::Test {
protected:

  LibraryInitializer() { }
  ~LibraryInitializer() override {}
  void SetUp() override {}
  void TearDown() override {}

  // called before executing the first test
  static void SetUpTestSuite() {
    auto &library = context.techMapContext.library;

    eda::gate::library::ReadCellsParser parser(LIBPATH);
    library = library::SCLibraryFactory::newLibraryUPtr(parser);

    if (library != nullptr) {
      std::cout << "Loaded Liberty file: " << LIBPATH << std::endl;
    } else {
      throw std::runtime_error("File loading failed");
    }
  }

  // called after the last test
  static void TearDownTestSuite() {
    context.techMapContext.library = nullptr;
  }

  static inline context::UtopiaContext context;
};

template <const path &LIBPATH>
class SubnetTechMapperTest : public LibraryInitializer<LIBPATH> {
protected:
  ~SubnetTechMapperTest() override {}

  const SubnetID commonPart(
      const std::shared_ptr<SubnetBuilder> builderPtr,
      const float maxArea,
      const float maxDelay,
      const float maxPower) {

    criterion::Objective objective(criterion::AREA);
    criterion::Constraints constraints = {
      criterion::Constraint(criterion::AREA, maxArea),
      criterion::Constraint(criterion::DELAY, maxDelay),
      criterion::Constraint(criterion::POWER, maxPower)
    };

    auto &context = LibraryInitializer<LIBPATH>::context;
    context.criterion =
      std::make_unique<criterion::Criterion>(objective, constraints);
    std::unique_ptr<optimizer::CutExtractor> cutExtractor{};

    auto matchFinder = [&](const SubnetBuilder &builder,
                           const optimizer::Cut &cut) {
      return pBoolMatcher_->match(builder, cut);
    };

    SubnetTechMapperPCut techmapper(
        "SubnetTechMapper",
        context,
        (*context.techMapContext.library).getProperties().maxArity,
        4, //maxCutNum
        matchFinder,
        estimator::getPPA);

    auto builder = techmapper.map(builderPtr);

    EXPECT_TRUE(builder != nullptr);

    if (builder != nullptr) {
      const auto mappedSubnetID = builder->make();
      EXPECT_TRUE(checkAllCellsMapped(mappedSubnetID));

      printStatistics(mappedSubnetID, *context.techMapContext.library);
      printVerilog(mappedSubnetID);
      std::cout << "Mapped Subnet: " << model::Subnet::get(mappedSubnetID);
      return mappedSubnetID;
    } else {
      return builderPtr->make(); // TODO
    }
  }

  void commonGenSubnetTests(
    SubnetID f(void), float area, float delay, float power) {
  const auto subnetID = f();
  const auto builderPtr = std::make_shared<SubnetBuilder>(subnetID);
  const auto mappedSubnetID = commonPart(builderPtr, area, delay, power);
  checkEQ(subnetID, mappedSubnetID);
}

protected:
  static void SetUpTestSuite() {
    LibraryInitializer<LIBPATH>::SetUpTestSuite();
    auto &library =
      LibraryInitializer<LIBPATH>::context.techMapContext.library;
    pBoolMatcher_ = std::move(
      Matcher<PBoolMatcher, kitty::dynamic_truth_table>::
        create(library->getCombCells()));
  }

  static void TearDownTestSuite() {
    LibraryInitializer<LIBPATH>::TearDownTestSuite();
    pBoolMatcher_ = nullptr;
  }

  std::string name_ = "UtopiaTechMapper";
  optimizer::CutExtractor *cutExtractor_ = nullptr;
  inline static std::unique_ptr<PBoolMatcher> pBoolMatcher_;
};

} // namespace eda::gate::techmapper
