//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>

namespace eda::util {

/// Counts units in the given 8-bit integer number.
inline uint8_t count_units(uint8_t x) {
  x = x - ((x >> 1) & 0x55);
  x = (x & 0x33) + ((x >> 2) & 0x33);
  x = (x + (x >> 4)) & 0x0F;
  x *= 0x01;
  return x;
}

/// Counts units in the given 16-bit integer number.
inline uint8_t count_units(uint16_t x) {
  x = x - ((x >> 1) & 0x5555);
  x = (x & 0x3333) + ((x >> 2) & 0x3333);
  x = (x + (x >> 4)) & 0x0F0F;
  x *= 0x0101;
  return x >> 8;
}

/// Counts units in the given 32-bit integer number.
inline uint8_t count_units(uint32_t x) {
  x = x - ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0F0F0F0F;
  x *= 0x01010101;
  return x >> 24;
}

/// Counts units in the given 64-bit integer number.
inline uint8_t count_units(uint64_t x) {
  x = x - ((x >> 1) & 0x5555555555555555ull);
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333ull);
  x = (x + (x >> 4)) & 0xF0F0F0F0F0F0F0Full;
  x *= 0x101010101010101ull;
  return x >> 56;
}

} // namespace eda::util
