//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/aigmapper.h"
#include "gate/premapper/premapper_test.h"

#include "gtest/gtest.h"

namespace eda::gate::premapper {

TEST(AigMapperTest, SimpleCases) {
  AigMapper aigmapper("aig");
  checkSimpleCases(&aigmapper);
}

TEST(AigMapperTest, ConstCases) {
  AigMapper aigmapper("aig");
  checkConstCases(&aigmapper);
}

TEST(AigMapperTest, MAJ) {
  AigMapper aigmapper("aig");
  checkMAJ(&aigmapper);
}

TEST(AigMapperTest, AND) {
  AigMapper aigmapper("aig");
  checkAND(&aigmapper);
}

TEST(AigMapperTest, OR) {
  AigMapper aigmapper("aig");
  checkOR(&aigmapper);
}

TEST(AigMapperTest, XOR) {
  AigMapper aigmapper("aig");
  checkXOR(&aigmapper);
}

TEST(AigMapperTest, RandomSubnet) {
  AigMapper aigmapper("aig");
  checkRandomSubnet(&aigmapper);
}

} // namespace eda::gate::premapper
