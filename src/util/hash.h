//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>

namespace eda::util {

/// Hash combination function as it is implemented in the Boost library.
/// Based on http://www.cs.rmit.edu.au/~jz/fulltext/jasist-tch.pdf. 
template <typename T>
inline void hash_combine(size_t &seed, const T &val) {
  seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed  << 6) + (seed >> 2);
}

} // namespace eda::util
