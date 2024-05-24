//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/subnet_basis.h"
#include "subnet_basis.h"

namespace eda::gate::optimizer {

SubnetBasis::SubnetBasis(IntegerType raw) {
  for (uint16_t i = 0; i < maxNumberOfElements; i++) {
    elements[i] = (raw >> i) & 1;
  }
}

SubnetBasis::SubnetBasis(std::initializer_list<BasisElement> elements) {
  for (const auto &el : elements) {
    setElement(el);
  }
}

bool SubnetBasis::hasElement(BasisElement el) const {
  return elements[static_cast<size_t>(el)];
}

void SubnetBasis::setElement(BasisElement el) {
  elements[static_cast<size_t>(el)] = true;
}

void SubnetBasis::unsetElement(BasisElement el) {
  elements[static_cast<size_t>(el)] = false;
}

SubnetBasis::operator IntegerType() const {
  IntegerType result = 0;
  for (uint16_t i = 0; i < maxNumberOfElements; i++) {
    if (elements[i]) {
      result = result | (1 << i);
    }
  }
  return result;
}

SubnetBasis SubnetBasis::operator|(const SubnetBasis &other) {
  SubnetBasis result;
  result.elements = elements | other.elements;
  return result;
}

SubnetBasis SubnetBasis::operator&(const SubnetBasis &other) {
  SubnetBasis result;
  result.elements = elements & other.elements;
  return result;
}

} // namespace eda::gate::optimizer
