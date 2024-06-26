//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <kitty/kitty.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace eda::utils {

//===----------------------------------------------------------------------===//
// Data types
//===----------------------------------------------------------------------===//

// General truth table.
using TruthTable = kitty::dynamic_truth_table;

// Specializations for 4, 5, and 6 variables.
using TruthTable4 = uint16_t;
using TruthTable5 = uint32_t;
using TruthTable6 = uint64_t;

// Shortcuts
using TTn = TruthTable;
using TT4 = TruthTable4;
using TT5 = TruthTable5;
using TT6 = TruthTable6;

//===----------------------------------------------------------------------===//
// Basic truth table functions
//===----------------------------------------------------------------------===//

template <typename TT /* TT4 or TT5 */>
TT getMaskTruthTable(size_t arity) {
  return (TT{1u} << (1 << arity)) - 1;
}

template <>
TT6 getMaskTruthTable<TT6>(size_t arity) {
  return arity == 6 ? -1ull : (1ull << (1 << arity)) - 1;
}

template <typename TT>
size_t getSizeTruthTable(const TT &tt) {
  assert(false && "Specialization is required");
}

template <>
size_t getSizeTruthTable<TTn>(const TTn &tt) {
  return tt.num_bits();
}

template <>
size_t getSizeTruthTable<TT4>(const TT4 &tt) {
  return 16;
}

template <>
size_t getSizeTruthTable<TT5>(const TT5 &tt) {
  return 32;
}

template <>
size_t getSizeTruthTable<TT6>(const TT6 &tt) {
  return 64;
}

template <typename TT>
bool getBitTruthTable(const TT &tt, size_t i) {
  assert(false && "Specialization is required");
}

template <>
bool getBitTruthTable<TTn>(const TTn &tt, size_t i) {
  return kitty::get_bit(tt, i);
}

template <>
bool getBitTruthTable<TT4>(const TT4 &tt, size_t i) {
  assert(i < 16);
  return (tt >> i) & 1;
}

template <>
bool getBitTruthTable<TT5>(const TT5 &tt, size_t i) {
  assert(i < 32);
  return (tt >> i) & 1;
}

template <>
bool getBitTruthTable<TT6>(const TT6 &tt, size_t i) {
  assert(i < 64);
  return (tt >> i) & 1;
}

template <typename TT>
void setBitTruthTable(TT &tt, size_t i) {
  assert(false && "Specialization is required");
}

template <>
void setBitTruthTable<TTn>(TTn &tt, size_t i) {
  kitty::set_bit(tt, i);
}

template <>
void setBitTruthTable<TT4>(TT4 &tt, size_t i) {
  assert(i < 16);
  tt |= (1u << i);
}

template <>
void setBitTruthTable<TT5>(TT5 &tt, size_t i) {
  assert(i < 32);
  tt |= (1u << i);
}

template <>
void setBitTruthTable<TT6>(TT6 &tt, size_t i) {
  assert(i < 64);
  tt |= (1ull << i);
}

template <typename TT>
void clearTruthTable(TT &tt) {
  assert(false && "Specialization is required");
}

template <>
void clearTruthTable<TTn>(TTn &tt) {
  kitty::clear(tt);
}

template <>
void clearTruthTable<TT4>(TT4 &tt) {
  tt = 0;
}

template <>
void clearTruthTable<TT5>(TT5 &tt) {
  tt = 0;
}

template <>
void clearTruthTable<TT6>(TT6 &tt) {
  tt = 0;
}

template <typename TT>
TT getZeroTruthTable(size_t arity) {
  assert(false && "Specialization is required");
  return TT{};
}

template <>
TTn getZeroTruthTable<TTn>(size_t arity) {
  auto tt = kitty::create<TTn>(arity);
  kitty::clear(tt);
  return tt;
}

template <>
TT4 getZeroTruthTable<TT4>(size_t arity) {
  return 0;
}

template <>
TT5 getZeroTruthTable<TT5>(size_t arity) {
  return 0;
}

template <>
TT6 getZeroTruthTable<TT6>(size_t arity) {
  return 0;
}

template <typename TT>
TT getOneTruthTable(const size_t arity) {
  return ~getZeroTruthTable<TT>(arity);
}

template <typename TT>
TT getVarTruthTable(size_t arity, size_t i) {
  assert(false && "Specialization is required");
}

template <>
TTn getVarTruthTable<TTn>(size_t arity, size_t i) {
  auto tt = kitty::create<TTn>(arity);
  kitty::create_nth_var(tt, i);
  return tt;
}

template <>
TT4 getVarTruthTable<TT4>(size_t arity, size_t i) {
  static TT4 vars[4] = {
    0xAAAAu,
    0xCCCCu,
    0xF0F0u,
    0xFF00u
  };

  assert(arity <= 4);
  return vars[i];
}

template <>
TT5 getVarTruthTable<TT5>(size_t arity, size_t i) {
  static TT5 vars[5] = {
    0xaaaaAAAAu,
    0xccccCCCCu,
    0xF0F0F0F0u,
    0xFF00FF00u,
    0xFFFF0000u
  };

  assert(arity <= 5);
  return vars[i];
}

template <>
TT6 getVarTruthTable<TT6>(size_t arity, size_t i) {
  static TT6 vars[6] = {
    0xaaaaAAAAaaaaAAAAull,
    0xccccCCCCccccCCCCull,
    0xF0F0F0F0F0F0F0F0ull,
    0xFF00FF00FF00FF00ull,
    0xFFFF0000FFFF0000ull,
    0xffffFFFF00000000ull
  };

  assert(arity <= 6);
  return vars[i];
}

template <typename TT>
TTn convertTruthTable(const TT &tt, size_t arity) {
  assert(false && "Specialization is required");
  return TT{};
}

template <>
TTn convertTruthTable<TTn>(const TTn &tt, size_t arity) {
  return tt;
}

template <>
TTn convertTruthTable<TT4>(const TT4 &tt, size_t arity) {
  auto res = kitty::create<TTn>(arity);
  *res.begin() = tt & getMaskTruthTable<TT4>(arity);
  return res;
}

template <>
TTn convertTruthTable<TT5>(const TT5 &tt, size_t arity) {
  auto res = kitty::create<TTn>(arity);
  *res.begin() = tt & getMaskTruthTable<TT5>(arity);
  return res;
}

template <>
TTn convertTruthTable<TT6>(const TT6 &tt, size_t arity) {
  auto res = kitty::create<TTn>(arity);
  *res.begin() = tt & getMaskTruthTable<TT6>(arity);
  return res;
}

//===----------------------------------------------------------------------===//
// Truth table calculator
//===----------------------------------------------------------------------===//

using SB   = gate::model::SubnetBuilder;
using Link = gate::model::Subnet::Link;
using Cell = gate::model::Subnet::Cell;

template <typename TT>
const TT &getTruthTable(const SB &builder, size_t i) {
  assert(false && "Specialization is required");
  static TT tt{};
  return tt;
}

template <>
const TTn &getTruthTable<TTn>(const SB &builder, size_t i) {
  assert(builder.isMarked(i));
  return *builder.getDataPtr<TTn>(i);
}

template <>
const TT4 &getTruthTable<TT4>(const SB &builder, size_t i) {
  assert(builder.isMarked(i));
  return builder.getDataVal<TT4>(i);
}

template <>
const TT5 &getTruthTable<TT5>(const SB &builder, size_t i) {
  assert(builder.isMarked(i));
  return builder.getDataVal<TT5>(i);
}

template <>
const TT6 &getTruthTable<TT6>(const SB &builder, size_t i) {
  assert(builder.isMarked(i));
  return builder.getDataVal<TT6>(i);
}

template <typename TT>
void setTruthTable(SB &builder, size_t i, const TT &tt) {
  assert(false && "Specialization is required");
}

template <>
void setTruthTable<TTn>(SB &builder, size_t i, const TTn &tt) {
  builder.setDataPtr(i, &tt /* Data should be alive */);
}

template <>
void setTruthTable<TT4>(SB &builder, size_t i, const TT4 &tt) {
  builder.setDataVal<TT4>(i, tt);
}

template <>
void setTruthTable<TT5>(SB &builder, size_t i, const TT5 &tt) {
  builder.setDataVal<TT5>(i, tt);
}

template <>
void setTruthTable<TT6>(SB &builder, size_t i, const TT6 &tt) {
  builder.setDataVal<TT6>(i, tt);
}

template <typename TT>
TT getTruthTable(const SB &builder, const Link &link) {
  const auto &tt = getTruthTable<TT>(builder, link.idx);
  return link.inv ? ~tt : tt;
}

template <typename TT>
TT getTruthTable(const SB &builder, size_t i, size_t j) {
  return getTruthTable<TT>(builder, builder.getLink(i, j));
}

template <typename TT>
TT getInTruthTable(size_t arity, size_t i) {
  return getVarTruthTable<TT>(arity, i);
}

template <typename TT>
TT getBufTruthTable(const SB &builder, const Cell &cell) {
  return getTruthTable<TT>(builder, cell.link[0]);
}

template <typename TT>
TT getAndTruthTable(const SB &builder, const Cell &cell, size_t i) {
  auto tt = getTruthTable<TT>(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt &= getTruthTable<TT>(builder, i, j);
  }
  return tt;
}

template <typename TT>
TT getOrTruthTable(const SB &builder, const Cell &cell, size_t i) {
  auto tt = getTruthTable<TT>(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt |= getTruthTable<TT>(builder, i, j);
  }
  return tt;
}

template <typename TT>
TT getXorTruthTable(const SB &builder, const Cell &cell, size_t i) {
  auto tt = getTruthTable<TT>(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt ^= getTruthTable<TT>(builder, i, j);
  }
  return tt;
}

template <typename TT>
TT getMajTruthTable(const SB &builder, const Cell &cell, size_t i) {
  auto tt = getTruthTable<TT>(builder, cell.link[0]);
  clearTruthTable<TT>(tt);

  std::vector<TT> args(cell.arity);
  for (size_t j = 0; j < cell.arity; ++j) {
    args[j] = getTruthTable<TT>(builder, i, j);
  }

  const unsigned threshold = cell.arity >> 1;
  for (size_t k = 0; k < getSizeTruthTable<TT>(tt); ++k) {
    unsigned count = 0;
    for (size_t j = 0; j < cell.arity; ++j) {
      if (getBitTruthTable<TT>(args[j], k)) count++;
    }
    if (count > threshold) {
      setBitTruthTable<TT>(tt, k);
    }
  }

  return tt;
}

template <typename TT>
TT getTruthTable(
    const SB &builder, size_t arity, size_t i, bool isIn, size_t nIn) {
  const auto &cell = builder.getCell(i);

  if (isIn) {
    return getInTruthTable<TT>(arity, nIn);
  }
  if (cell.isZero()) {
    return getZeroTruthTable<TT>(arity);
  }
  if (cell.isOne()) {
    return getOneTruthTable<TT>(arity);
  }
  if (cell.isOut()) {
    return getBufTruthTable<TT>(builder, cell);
  }
  if (cell.isBuf()) {
    return getBufTruthTable<TT>(builder, cell);
  }
  if (cell.isAnd()) {
    return getAndTruthTable<TT>(builder, cell, i);
  }
  if (cell.isOr()) {
    return getOrTruthTable<TT>(builder, cell, i);
  }
  if (cell.isXor()) {
    return getXorTruthTable<TT>(builder, cell, i);
  }
  if (cell.isMaj()) {
    return getMajTruthTable<TT>(builder, cell, i);
  }

  assert(false && "Unsupported operation");
  return getZeroTruthTable<TT>(arity);
}

} // namespace eda::utils
