//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "reed_muller.h"
#include <algorithm>
#include <cassert>

namespace eda::gate::optimizer::synthesis {
    
ReedMuller::ReedMuller(uint64_t (*metricFunction)(Polynomial &)) : 
  currentMetricFunction(metricFunction) { }

SubnetObject ReedMuller::synthesize(const TruthTable& func,
                                    const TruthTable &, uint16_t maxArity) const {
  uint64_t len = func.num_bits();
  variables = func.num_vars();

  for (uint64_t i = 0; i < len; ++i) {
    polynomial.push_back(kitty::get_bit(func, i));
  }

  for (uint64_t index = 0; index < variables; ++index) {
    polarize(index);
  }

  PolarizedPolynomial result;
  if (currentMetricFunction) {
    result = getOptimal();
  } else {
    result.first = polynomial;
    result.second = Polarization(polynomial.size(), false);
  }

  return createScheme(result.first, result.second, maxArity, variables);
}

void ReedMuller::polarize(uint64_t index) const {
  polarityOperation(index, true);
}

void ReedMuller::changePolarity(uint64_t index) const {
  polarityOperation(index, false);
}

void ReedMuller::polarityOperation(uint64_t index, bool rightShift) const {
  Polynomial copy = polynomial;
  for (uint64_t i = 0; i < polynomial.size(); ++i) {
    if ((!(i & (1 << index)) && !rightShift) || (i & (1 << index) && rightShift)) {
      polynomial[i] = 0;
    }
  }

  shift(1 << index, rightShift);

  auto tmp = copy;
  copy = polynomial;
  polynomial = tmp;

  /* In C++ 20 this can be done easier:
   * std::shift_left(copy.begin(), copy.end(), 1 << index);
   */

  for (uint64_t i = 0; i < copy.size(); ++i) {
    polynomial[i] ^= copy[i];
  }
}

uint64_t ReedMuller::getPolarized() const {
  bool flag = true;
  uint64_t minMetric = currentMetricFunction(polynomial);
  uint64_t version = 0;
  Polynomial optimum = polynomial;
  for (uint64_t i = 0; i < polynomial.size(); ++i) {
    uint64_t firstOnePos = 0;
    while (!((i + 1) & (1 << firstOnePos))) {
      ++firstOnePos;
    }
    changePolarity(firstOnePos);

    uint64_t curMetric = currentMetricFunction(polynomial);
    if (curMetric < minMetric) {
      flag = false;
      minMetric = curMetric;
      optimum = polynomial;
      version = i;
    }
  }
  polynomial = optimum;
  return flag ? 0 : version + 1;
}

PolarizedPolynomial ReedMuller::getOptimal() const {
  uint64_t step = getPolarized();
  std::vector<bool> polarized(variables, false);
  for (uint64_t i = 0; (i < step) && step; ++i) {
    uint64_t firstOnePos = 0;
    while (!((i + 1) & (1 << firstOnePos))) {
      ++firstOnePos;
    }
    polarized[firstOnePos] = !polarized[firstOnePos];
  }
  PolarizedPolynomial result = {polynomial, polarized};
  return result;
}

void ReedMuller::shift(uint64_t pos, bool right) const {
  uint64_t len = polynomial.size();
  pos = right ? pos : len - pos;
  pos %= len;

  std::reverse(polynomial.begin(), polynomial.begin() + (len - pos));
  std::reverse(polynomial.begin() + (len - pos), polynomial.end());
  std::reverse(polynomial.begin(), polynomial.end());    
}

} // namespace eda::gate::optimizer::synthesis
