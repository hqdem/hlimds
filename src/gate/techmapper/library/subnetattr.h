//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include <string>
#include <vector>

namespace eda::gate::techmapper {
struct Power {
  float fall_power;
  float rise_power;
};

struct Subnetattr {
  std::string name;
  float area;
  std::vector <Power> pinsPower;
  size_t fanout_count = 1;
  };
} // namespace eda::gate::techmapper
