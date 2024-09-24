//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library_factory.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "gate/techmapper/matcher/pbool_matcher.h"

#include "gate/techmapper/techmapper_test_util.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

namespace eda::gate::techmapper {

class MatcherTest : public testing::Test {
protected:
  MatcherTest() {
    auto &library = context_.techMapContext.library;
    eda::gate::library::ReadCellsParser parser(techLib);
    library = library::SCLibraryFactory::newLibraryUPtr(parser);

    if (library != nullptr) {
      std::cout << "Loaded Liberty file: " << techLib << std::endl;
    } else {
      throw std::runtime_error("File loading failed");
    }
    pBoolMatcher_ = std::move(
        Matcher<PBoolMatcher, kitty::dynamic_truth_table>::create(
          library->getCombCells()));
  }

  ~MatcherTest() override {}
  void SetUp() override {}
  void TearDown() override {}

  std::unique_ptr<PBoolMatcher> pBoolMatcher_ {};
  eda::context::UtopiaContext context_;
};

TEST_F(MatcherTest, RandomTruthTable) {
  for (uint i = 0; i < 100; i++) {
    kitty::dynamic_truth_table tt(2);
    kitty::create_random(tt);
    auto config = kitty::exact_p_canonization(tt);
    const auto &ctt = util::getTT(config); // canonized TT
    std::vector<std::pair<library::StandardCell, uint16_t>> scs;
    pBoolMatcher_->match(scs, ctt);
    if(scs.empty()) {
      std::cout << "truth table " << kitty::to_hex(tt) <<
                         " (ctt=" << kitty::to_hex(ctt) << ")" <<
                         " is not matched." << std::endl;
    }
    EXPECT_TRUE(!scs.empty());
  }
}
} // namespace eda::gate::techmapper