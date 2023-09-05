//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/util.h"

namespace eda::utils {

std::string toBinString(int num, uint64_t size) {
  std::string res;

  uint64_t index = 0;

  if (!num) {
    while (index++ < size) {
      res.push_back('0');
    }
  } else {
    while (num) {
      res.push_back((num % 2) + '0');
      num /= 2;
      ++index;
    }
    while (index++ < size) {
      res.push_back('0');
    }
    std::reverse(res.begin(), res.end());
  }
  return res;
}

} // namespace eda::utils
