//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "kitty_utils.h"

namespace eda::utils {

using TT = kitty::dynamic_truth_table;

TT toTT(uint64_t x) {
  TT tt(6);
  *tt.begin() = x;
  return tt;
}

gate::model::SubnetID npnTransform(const gate::model::Subnet &subnet,
                                   const NPNTransformation &t) {
  using Cell = gate::model::Subnet::Cell;
  using Subnet = gate::model::Subnet;
  using SubnetObject = gate::model::SubnetObject;

  uint16_t negationMask = t.negationMask;
  NPNTransformation::InputPermutation permutation = t.permutation;

  SubnetObject object;
  auto &builder = object.builder();

  const auto &entries = subnet.getEntries();

  // Checking inputs
  size_t expectedInputCount = permutation.size();
  assert(entries.size() >= expectedInputCount);

  NPNTransformation::InputPermutation rPermutation(permutation.size());
  for (size_t i = 0; i < permutation.size(); i++) {
    rPermutation[permutation[i]] = i;
  }

  for (size_t i = 0; i < entries.size(); ++i) {
    const Cell &cell = entries[i].cell;
    if (i < expectedInputCount) {
      assert(cell.isIn() &&
             "Subnet inputs count doesn't match permutation size.");
    }

    Subnet::LinkList links(cell.link, cell.link + cell.arity);
    for (auto &link : links) {
      size_t idx = link.idx;
      if (idx < expectedInputCount) {
        link.idx = rPermutation[idx];
        if (((negationMask >> idx) & 1) == 1) {
          link.inv = 1 - link.inv;
        }
      }
      if (cell.isOut() && (((negationMask >> expectedInputCount) & 1) == 1)) {
        link.inv = 1 - link.inv;
      }
    }
    builder.addCell(cell.getTypeID(), links);
  }

  return object.make();
}

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

KittyTT generateConstTT(size_t numVars, bool on) {
  KittyTT tt(numVars);
  kitty::create_from_binary_string(
      tt, std::string(1ull << numVars, on ? '1' : '0'));
  return tt;
}

} // namespace eda::utils
