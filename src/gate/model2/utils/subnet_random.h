//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include "kitty/kitty.hpp"

#include <cassert>
#include <random>
#include <vector>

namespace eda::gate::model {

inline SubnetID randomSubnet(
    size_t nIn, size_t nOut, size_t nCell, size_t minArity, size_t maxArity) {
  assert(nIn > 0 && nOut > 0);
  assert(nCell >= (nIn + nOut));
  assert(minArity <= maxArity);

  SubnetBuilder builder;
  std::vector<size_t> cells(nCell);

  for (size_t i = 0; i < nIn; ++i) {
    cells[i] = builder.addCell(IN, SubnetBuilder::INPUT);
  }

  // Arity distribution.
  const auto nArities = (maxArity - minArity) + 1;
  std::uniform_int_distribution<size_t> arity(minArity, maxArity);

  // Gate distribution.
  static CellSymbol symbols[] = { AND, OR, XOR, NAND, NOR, XNOR, MAJ };
  const auto nSymbols = sizeof(symbols) / sizeof(CellSymbol);
  const auto isMajAllowed = (nArities > 1) || (minArity & 1);

  const auto minSymbolIdx = 0u;
  const auto maxSymbolIdx = isMajAllowed ? nSymbols - 1 : nSymbols - 2;
  std::uniform_int_distribution<size_t> symbolIdx(minSymbolIdx, maxSymbolIdx);

  // Inverter distribution.
  std::bernoulli_distribution inverter(0.5);

  std::random_device device;
  std::mt19937 generator(device());

  for (size_t i = nIn; i < nCell; ++i) {
    auto kind = (i >= (nCell - nOut)) ? SubnetBuilder::OUTPUT
                                      : SubnetBuilder::INNER;
    auto f = symbols[symbolIdx(generator)];
    auto k = arity(generator);

    if (f == MAJ && (k & 1) == 0) {
      k = k < maxArity ? k + 1 : k - 1;
    }

    // Link distribution.
    std::uniform_int_distribution<size_t> cellIdx(0, i - 1);

    Subnet::LinkList links(k);
    for (size_t j = 0; j < k; ++j) {
      auto idx = cellIdx(generator);
      auto inv = inverter(generator);
      links[j] = Subnet::Link(cells[idx], inv);
    }

    cells[i] = builder.addCell(f, links, kind);
  }

  return builder.make();
}

} // namespace eda::gate::model
