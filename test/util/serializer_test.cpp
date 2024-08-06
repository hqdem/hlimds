//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/serializer.h"

#include "gtest/gtest.h"

namespace eda::util {

TEST(SerializerTest, MapSerializerTest) {
  std::unordered_map<int, int> m1 = {
    {-5, 2},
    {42, 0}
  };
  std::unordered_map<int, char> m2 = {
    {142, 'a'},
    {-15, '/'}
  };
  std::stringstream ss;
  MapSerializer<int, int> s1;
  MapSerializer<int, char> s2;
  s1.serialize(ss, m1);
  s1.serialize(ss, m1);
  s2.serialize(ss, m2);

  ASSERT_EQ(m1, s1.deserialize(ss));
  ASSERT_EQ(m1, s1.deserialize(ss));
  ASSERT_EQ(m2, s2.deserialize(ss));
}

} // namespace eda::utils
