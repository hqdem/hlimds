//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "context/utopia_context.h"
#include "gate/library/library_factory.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "gate/estimator/simple_time_model.h"
#include "util/env.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>

namespace eda::gate::estimator {

using path = std::filesystem::path;

class EstimatorTest : public testing::Test {
protected:
  EstimatorTest() { //TODO: make common library init part for tests to use
    const path homePath = eda::env::getHomePath();
    const path filePath = homePath / libName;
    auto &library = context_.techMapContext.library; 
    eda::gate::library::ReadCellsParser parser(filePath);
    library = library::SCLibraryFactory::newLibraryUPtr(parser);

    if (library != nullptr) {
      std::cout << "Loaded Liberty file: " << libName << std::endl;
    } else {
      throw std::runtime_error("File loading failed");
    }

  }

  const library::StandardCell* findCellByName(const std::string &cellName) {
    const auto &cells = context_.techMapContext.library->getCombCells();
    auto it = find_if(cells.begin(), cells.end(),
                  [&] (const auto& c) { return c.name == cellName; });
    if (it != cells.end()) {
      return &(*it);
    }
    return nullptr;
  }

  void commonPart(const std::string &cellTypeName,
    const double inputTransTime, const double totalOutputCap,
    const double slewRef, const double delayRef) {

    int timingSense = 0;
    double slew = 0, delay = 0, cap = 0;
    
    const auto *cellPtr = findCellByName(cellTypeName);

    if (cellPtr == nullptr) {
      std::cout << "!!!Did not find cell: "
                << cellTypeName << " in library!!!\n";
      ASSERT_TRUE(false);
    }

    NLDM::delayEstimation(*cellPtr,
                          inputTransTime,
                          totalOutputCap,
                          timingSense, slew, delay, cap);
    EXPECT_FLOAT_EQ(slew, slewRef); // FIXME: use EXPECT_DOUBLE_EQ
    EXPECT_FLOAT_EQ(delay, delayRef); // FIXME: use EXPECT_DOUBLE_EQ
  }

  ~EstimatorTest() override {}
  void SetUp() override {}
  void TearDown() override {}

  std::string libName = "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";
  eda::context::UtopiaContext context_;
};

TEST_F(EstimatorTest, a2111o4) {
  std::string cellName = "sky130_fd_sc_hd__a2111o_4";
  commonPart(cellName, 0.053133, 0.191204, 0.37748932838439941, 0.4451143741607666);
}

TEST_F(EstimatorTest, o21a4) {
  std::string cellName = "sky130_fd_sc_hd__o21a_4";
  commonPart(cellName, 0.053133, 0.001627, 0.024740446358919144, 0.09489276260137558);
}

TEST_F(EstimatorTest, a211o2) {
  std::string cellName = "sky130_fd_sc_hd__a211o_2";
  commonPart(cellName, 0.099999, 0.002468, 0.034578997641801834, 0.11193791031837463);
}

} // eda::gate::techmapper
