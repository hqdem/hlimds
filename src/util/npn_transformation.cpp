//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "npn_transformation.h"

namespace eda::utils {

NpnTransformation inverse(const NpnTransformation &t) {
  uint32_t negationMask = 0;
  NpnTransformation::InputPermutation
    permutation = NpnTransformation::InputPermutation(t.permutation.size());
  for (size_t i = 0; i < permutation.size(); i++) {
    permutation[t.permutation[i]] = i;
    if (t.negationMask & (1 << t.permutation[i])) {
      negationMask = negationMask | (1 << i);
    }
  }
  negationMask = negationMask | (t.negationMask & (1 << permutation.size()));
  return NpnTransformation{negationMask, permutation};
}

} // namespace eda::utils
