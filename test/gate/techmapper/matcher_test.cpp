//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/matcher/pbool_matcher.h"

#include "gate/techmapper/techmapper_test_util.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

namespace eda::gate::techmapper {

PBoolMatcher *pBoolMatcher = nullptr;

void commonPart() {
  // boolMatcher is created once the library is loaded.
  if (pBoolMatcher == nullptr) {
    library::library = new library::SCLibrary(techLib);
    if (library::library != nullptr) {
      std::cout << "Loaded Liberty file: " << techLib << std::endl;
    }
    pBoolMatcher = Matcher<PBoolMatcher, kitty::dynamic_truth_table>::create(
      library::library->getCombCells());
  }
}

TEST(MatcherTest, RandomTruthTable) {
  commonPart();
  for (uint i = 0; i < 100; i++) {
    kitty::dynamic_truth_table tt(2);
    kitty::create_random(tt);
    auto config = kitty::exact_p_canonization(tt);
    const auto &ctt = util::getTT(config); // canonized TT
    std::vector<library::SCLibrary::StandardCell> scs;
    pBoolMatcher->match(scs, ctt);
    if(scs.empty()) {
      std::cout << "truth table " << kitty::to_hex(tt) <<
                         " (ctt=" << kitty::to_hex(ctt) << ")" <<
                         " is not matched." << std::endl;
    }
    EXPECT_TRUE(!scs.empty());
  }
}
} // namespace eda::gate::techmapper