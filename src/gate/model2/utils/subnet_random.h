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
  Subnet::LinkList subnetLinks = builder.addInputs(nIn);
  subnetLinks.reserve(nCell);

  // Arity distribution.
  const auto nArities = (maxArity - minArity) + 1;
  std::uniform_int_distribution<size_t> arityDist(minArity, maxArity);

  // Gate distribution.
  static CellSymbol symbols[] = { AND, OR, XOR, MAJ };
  const auto nSymbols = sizeof(symbols) / sizeof(CellSymbol);
  const auto isMajAllowed = (nArities > 1) || (minArity & 1);

  const auto minSymbol = 0u;
  const auto maxSymbol = isMajAllowed ? nSymbols - 1 : nSymbols - 2;
  std::uniform_int_distribution<size_t> symbolDist(minSymbol, maxSymbol);

  // Inverter distribution.
  std::bernoulli_distribution inverter(0.5);

  std::random_device device;
  std::mt19937 generator(device());

  for (size_t i = nIn; i < (nCell - nOut); ++i) {
    const auto symbol = symbols[symbolDist(generator)];
    auto k = arityDist(generator);

    if (symbol == MAJ && (k & 1) == 0) {
      k = k < maxArity ? k + 1 : k - 1;
    }

    // Link distribution.
    std::uniform_int_distribution<size_t> linkDist(0, i - 1);

    Subnet::LinkList links(k);
    for (size_t j = 0; j < k; ++j) {
      const auto idx = linkDist(generator);
      const auto inv = inverter(generator);
      links[j] = inv ? ~subnetLinks[idx] : subnetLinks[idx];
    }

    subnetLinks.push_back(builder.addCell(symbol, links));
  }

  // Link distribution.
  std::uniform_int_distribution<size_t> linkDist(0, subnetLinks.size() - 1);
 
  for (size_t i = 0; i < nOut; ++i) {
    const auto idx = linkDist(generator);
    const auto inv = inverter(generator);
    builder.addOutput(inv ? ~subnetLinks[idx] : subnetLinks[idx]);
  }

  return builder.make();
}

} // namespace eda::gate::model
