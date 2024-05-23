//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "probabilistic_estimate.h"

namespace eda::gate::analyzer {

float ProbabilisticEstimate::combinations(
    size_t k, size_t n, std::vector<float> &prob) const {
  
  float p = 0.0;

  for (size_t i = 0; i < (1ul << n); ++i) {
    std::bitset<32> bits(i);

    if (bits.count() == k) {
      float pComb = 1.0;

      for (size_t j = 0; j < n; ++j) {
        pComb = pComb * (bits[j] ? prob[j] : (1 - prob[j]));
      }

      p += pComb;
    }
  }
  
  return p;
}

float ProbabilisticEstimate::majEstimate(
    std::vector<float> &majProb, size_t nMajP) const {

  float p = 0.0;

  for (size_t i = nMajP/2 + 1; i <= nMajP; i++) {
    p = p + combinations(i, nMajP, majProb);
  }

  return p;
}

float ProbabilisticEstimate::xorEstimate(
    std::vector<float> &xorProb, size_t nXorP) const {

  float p = 0.0;
    
  for (size_t i = 1; i <= nXorP; i = i + 2) {
    p = p + combinations(i, nXorP, xorProb);
  }

  return p;
}

std::vector<float> ProbabilisticEstimate::probEstimator(const Subnet &subnet,
    const Probabilities &probabilities) const {
  
  uint32_t lenInputProb = probabilities.size();
  size_t lenArrEst = subnet.size();
  std::vector<float> arrProbability;
  std::vector<float> cellEstimate;
  cellEstimate.resize(lenArrEst);;
  uint32_t k = 0;
  const auto cells = subnet.getEntries();
  
  for (size_t i = 0; i < lenArrEst; ++i) {
    float p = 1.0;
    const auto &cell = cells[i].cell;
    auto flag = false;
    
    if (cell.isIn()) {
      flag = true;
      p = (k < lenInputProb) ? probabilities[k] : 0.5;
      k++;
    }

    if (cell.isAnd()) {
      flag = true;

      for (size_t j = 0; j < cell.arity; ++j) {
        auto link = subnet.getLink(i, j);
        auto cellEst = cellEstimate[link.idx];
        p = p * (link.inv ? (1 - cellEst) : cellEst);
      }

    }

    if (cell.isOr()) {
      flag = true;

      for (size_t j = 0; j < cell.arity; ++j) {
        auto link = subnet.getLink(i, j);
        auto cellEst = cellEstimate[link.idx];
        p = p * (link.inv ? cellEst : (1 - cellEst));
      }

      p = 1 - p;
    }

    if (cell.isOne()) {
      flag = true;
      p = 1.0;
    }

    if (cell.isZero()) {
      flag = true;
      p = 0.0;
    }

    if (cell.isBuf()) {
      flag = true;
      auto link = subnet.getLink(i, 0);
      p = link.inv ? (1 - cellEstimate[link.idx]) : cellEstimate[link.idx];
    }

    if (cell.isMaj() or cell.isXor()) {
      flag = true;
      
      for (size_t j = 0; j < cell.arity; ++j) {
        auto link = subnet.getLink(i, j);
        auto cellEst = cellEstimate[link.idx];
        auto cellPr = link.inv ? (1 - cellEst) : cellEst;
        arrProbability.push_back(cellPr);
      }

      size_t nArrProb = arrProbability.size();

      p = cell.isMaj() ? majEstimate(arrProbability, nArrProb)
          : xorEstimate(arrProbability, nArrProb);
    }

    if (cell.isOut()) {
      flag = true;
      auto link = subnet.getLink(i, 0);
      p = link.inv ? (1 - cellEstimate[link.idx]) : cellEstimate[link.idx];
    }

    if (not flag) {
      p = 0;
    }

    cellEstimate[i] = p;

    arrProbability.clear();
  }

  return cellEstimate;
}

SwitchActivity ProbabilisticEstimate::estimate(const Subnet &subnet,
    const Probabilities &probabilities) const {

  Probabilities onState = probEstimator(subnet, probabilities);
  Probabilities switching{onState};
  const auto &entries = subnet.getEntries();

  for (size_t j = 0; j < entries.size(); ++j) {
    float p = switching[j];
    const auto &cell = entries[j].cell;
    switching[j] = cell.isBuf() ? 0.f : (2.f * p * (1.f - p));
  }

  return SwitchActivity(std::move(switching), std::move(onState));
}

} // namespace eda::gate::analyzer
