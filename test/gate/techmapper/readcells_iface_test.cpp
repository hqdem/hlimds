//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "context/utopia_context.h"
#include "gate/library/library.h"
#include "gate/library/readcells_iface.h"
#include "util/env.h"

#include "gtest/gtest.h"

using path = std::filesystem::path;

const path home = eda::env::getHomePath();
const path techLibPath = home /
                         "test/data/gate/techmapper";

namespace eda::gate::library {

ReadCellsIface::Delay checkDelayInterpolation(library::SCLibrary &library) {
  
  ReadCellsIface iface(library.getLibraryRaw());

  auto delay = iface.getDelay("sky130_fd_sc_hd__nor2b_1",
                                                     "A",
                                                   0.122,
                                                 0.00291);

//#ifdef UTOPIA_DEBUG
  std::cout << delay.cellRise << std::endl;
  std::cout << delay.cellFall << std::endl;
  std::cout << delay.riseTransition << std::endl;
  std::cout << delay.fallTransition << std::endl;
//#endif // UTOPIA_DEBUG
  return delay;
}

class ReadCellsIfaceTest : public testing::Test {
protected:
  ReadCellsIfaceTest() {
    //TODO: remove EXPECT_EXIT once logic basis completion is performed
    EXPECT_EXIT({
      context_.techMapContext.library =
        std::make_unique<library::SCLibrary>(home_ / techLibPath_);
    },::testing::KilledBySignal(SIGABRT), "");

    if (context_.techMapContext.library != nullptr) {
      std::cout << "Loaded Liberty file: " << home_ / techLibPath_ << std::endl;
    } else {
      throw std::runtime_error("File loading failed");
    }
  }

  ~ReadCellsIfaceTest() override {}
  void SetUp() override {}
  void TearDown() override {}

  const path home_ = eda::env::getHomePath();
  const path techLibPath_ = "test/data/gate/techmapper/test_nor.lib";
  eda::context::UtopiaContext context_;
};

TEST_F(ReadCellsIfaceTest, DelayInterpolation) {
  auto delay = checkDelayInterpolation(*context_.techMapContext.library);
  bool isCorrect = false;

  if (0.0769735000 < delay.cellRise && delay.cellRise < 0.1284223000) {
    isCorrect = true;
  }
  EXPECT_TRUE(isCorrect);
}

} // namespace eda::gate::techmapper
