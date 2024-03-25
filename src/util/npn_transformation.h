//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace eda::utils {

/**
 * Represents NPN transformation.
 * Includes input/output negation and input permutation.
 * First (n-1) bits of negationMask are responsible for inputs negation.
 * n'th bit of negationMask stores output negation.
 * Where *n* is number of inputs.
 */
struct NPNTransformation {
  using InputPermutation = std::vector<uint8_t>;

  uint32_t negationMask;
  InputPermutation permutation;
};

/**
 * \brief Inverses NPN transformation.
 */
NPNTransformation inverse(const NPNTransformation &t);

} // namespace eda::utils
