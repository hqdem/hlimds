//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <bitset>
#include <cstdint>
#include <initializer_list>

namespace eda::gate::optimizer {

/**
 * @brief Represents subnet basis.
 */
struct SubnetBasis final {
  static constexpr uint16_t maxNumberOfElements = 16;

  using IntegerType = uint16_t;
  static_assert(sizeof(IntegerType) * 8 >= maxNumberOfElements);

  enum class BasisElement {
    AND = 0,
    MAJ = 1,
    XOR = 2,
    OR  = 3
  };

  std::bitset<maxNumberOfElements> elements;

  SubnetBasis() = default;
  SubnetBasis(std::initializer_list<BasisElement> elements);
  SubnetBasis(IntegerType raw);

  bool hasElement(BasisElement el) const;
  void setElement(BasisElement el);
  void unsetElement(BasisElement el);

  explicit operator IntegerType() const;

  SubnetBasis operator|(const SubnetBasis &other);
  SubnetBasis operator&(const SubnetBasis &other);
};

} // namespace eda::gate::optimizer
