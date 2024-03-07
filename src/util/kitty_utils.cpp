//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "kitty_utils.h"

namespace eda::utils {

SOP findAnyLevel0Kernel(const SOP &sop) {
  Cube lit = findAnyRepeatLiteral(sop);
  if (lit._mask == 0u) {
    return sop;
  }
  SOP quotient = findDivideByLiteralQuotient(sop, lit);
  makeCubeFree(quotient);
  return findAnyLevel0Kernel(quotient); 
}

Cube findAnyRepeatLiteral(const SOP &sop) {
  uint32_t onesBuf{0};
  uint32_t zerosBuf{0};
  uint32_t ones{0};
  uint32_t zeros{0};
  uint32_t repeatOnes{0};
  uint32_t repeatZeros{0};
  for (const Cube &cube : sop) {
    ones = cube._bits & cube._mask;
    zeros = ~cube._bits & cube._mask;
    repeatOnes = onesBuf & ones;
    repeatZeros = zerosBuf & zeros;
    if (repeatOnes) {
      uint32_t bit = FIRST_ONE_BIT(repeatOnes);
      return Cube{bit, bit};
    }
    if (repeatZeros) {
      uint32_t bit = FIRST_ONE_BIT(repeatZeros);
      return Cube{0u, bit};
    }
    onesBuf |= ones;
    zerosBuf |= zeros;
  }
  return Cube{};
}

SOP findDivideByLiteralQuotient(const SOP &sop, const Cube lit) {
  SOP quotient;
  for (const Cube &cube : sop) {
    if (cubeHasLiteral(cube, lit)) {
      Cube newCube = cube;
      newCube._bits &= ~lit._mask;
      newCube._mask &= ~lit._mask;
      quotient.push_back(newCube);
    }
  }
  return quotient;
}

Cube findCommonCube(const SOP &sop) {
  uint32_t ones{1u};
  uint32_t zeros{1u};
  for (const Cube &cube : sop) {
    ones &= (cube._bits & cube._mask);
    zeros &= (~cube._bits & cube._mask);
  }
  return Cube{ones, ones | zeros}; 
}

bool cubeFree(const SOP &sop) {
  return (findCommonCube(sop)._mask == 0u);
}

void makeCubeFree(SOP &sop) {
  Cube common = findCommonCube(sop);
  if (common._mask) {  
    for (Cube &cube : sop) {
      cube._mask &= ~common._mask;
      cube._bits &= ~common._mask;
    }
  }
}

Cube findBestLiteral(const SOP &sop, Cube lits) {
  size_t max{0u};
  Cube res;
  for (; lits._mask; lits._mask &= (lits._mask - 1)) {
    uint32_t bit = FIRST_ONE_BIT(lits._mask);
    Cube lit(lits._bits & bit, bit); 
    size_t count{0};
    for (Cube cube : sop) {
      if (cubeHasLiteral(cube, lit)) {
        ++count;
      }
    }
    if (count > max) {
      max = count;
      res = lit;
    }
  }
  assert(res._mask);
  return res;
}

bool cubeHasLiteral(const Cube cube, const Cube lit) {
  return (cube._mask & lit._mask) && ((cube._bits & lit._mask) == lit._bits);
}

bool cubeContain(Cube large, Cube small) {
  return ((large._mask & small._mask) == small._mask) &&
      ((large._bits & small._mask) == small._bits);
}

Cube cutCube(Cube large, Cube small) {
  return Cube{large._bits & ~small._mask, large._mask & ~small._mask};
}

Link synthFromSOP(const SOP &sop, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, uint16_t maxArity) {
  if (sop.size() == 1) {
    return synthFromCube(sop[0], inputs, subnetBuilder, maxArity);
  }

  LinkList links;
  for (auto it = sop.begin(); it < sop.end(); ++it) {
    Link link = synthFromCube(*it, inputs, subnetBuilder, maxArity);
    links.push_back(~link);
  }
  
  return ~subnetBuilder.addCellTree(CellSymbol::AND, links, maxArity);
}

Link synthFromCube(Cube cube, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, int16_t maxArity) {
  uint32_t mask {cube._mask};
  LinkList links;
  for (; mask; mask &= (mask - 1)) {
    size_t idx = std::log2(mask - (mask & (mask - 1)));
    bool inv = !(cube.get_bit(idx));
    links.push_back(Link(inputs[idx].idx, inv));
  }
  if (links.size() == 1) {
    return links[0];
  }
  return subnetBuilder.addCellTree(CellSymbol::AND, links, maxArity);
}

} // namespace eda::utils
