//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library_parser.h"
#include "gate/library/library_characteristics.h"
#include "util/env.h"

#include "gtest/gtest.h"

using path = std::filesystem::path;

const path home = eda::env::getHomePath();
const path techLibPath = home /
                         "test/data/gate/techmapper";

namespace eda::gate::library {
void loadLibrary(std::string libertyPath) {
  LibraryParser::get().loadLibrary(libertyPath);
}

library::LibraryCharacteristics::Delay checkDelayInterpolation(std::string libertyPath) {
  loadLibrary(libertyPath);
  auto delay = library::LibraryCharacteristics::getDelay("sky130_fd_sc_hd__nor2b_1",
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

TEST(LibraryCharacteristic, DelayInterpolation) {
auto delay = checkDelayInterpolation(techLibPath / "test_nor.lib");
bool isCorrect = false;

if (0.0769735000 < delay.cellRise && delay.cellRise < 0.1284223000) {
  isCorrect = true;
}
EXPECT_TRUE(isCorrect);
}

} // namespace eda::gate::techmapper
