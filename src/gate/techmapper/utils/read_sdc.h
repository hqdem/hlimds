//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <tuple>

namespace eda::gate::techmapper {

inline std::tuple<float, float, float> parseSDCFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file." << std::endl;
    return std::make_tuple(0.0f, 0.0f, 0.0f);
  }

  std::string line;
  std::regex delay_regex(R"(set_max_delay\s+(\d+(\.\d+)?))");
  std::regex area_regex(R"(set_max_area\s+(\d+(\.\d+)?))");
  std::regex dyn_power_regex(R"(set_max_dynamic_power\s+(\d+(\.\d+)?))");

  float max_delay = 0.0f;
  float max_area = 0.0f;
  float max_dynamic_power = 0.0f;

  while (std::getline(file, line)) {
    std::smatch match;
    if (std::regex_search(line, match, delay_regex)) {
      max_delay = std::stof(match[1]);
    } else if (std::regex_search(line, match, area_regex)) {
      max_area = std::stof(match[1]);
    } else if (std::regex_search(line, match, dyn_power_regex)) {
      max_dynamic_power = std::stof(match[1]);
    }
  }

  file.close();
  return std::make_tuple(max_delay, max_area, max_dynamic_power);
}
} // namespace eda::gate::techmapper
