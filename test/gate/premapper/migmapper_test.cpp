//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/migmapper.h"
#include "gate/premapper/premapper_test.h"

#include "gtest/gtest.h"

namespace eda::gate::premapper {

TEST(MigMapperTest, SimpleCases) {
  MigMapper migmapper("mig");
  checkSimpleCases(&migmapper);
}

TEST(MigMapperTest, ConstCases) {
  MigMapper migmapper("mig");
  checkConstCases(&migmapper);
}

TEST(MigMapperTest, MAJ) {
  MigMapper migmapper("mig");
  checkMAJ(&migmapper);
}

TEST(MigMapperTest, AND) {
  MigMapper migmapper("mig");
  checkAND(&migmapper);
}

TEST(MigMapperTest, OR) {
  MigMapper migmapper("mig");
  checkOR(&migmapper);
}

TEST(MigMapperTest, XOR) {
  MigMapper migmapper("mig");
  checkXOR(&migmapper);
}

TEST(MigMapperTest, RandomSubnet) {
  MigMapper migmapper("mig");
  checkRandomSubnet(&migmapper);
}

} // namespace eda::gate::premapper
