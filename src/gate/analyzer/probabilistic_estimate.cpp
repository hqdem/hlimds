//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "probabilistic_estimate.h"

namespace eda::gate::analyzer {

using CellActivities = std::vector<double>;
using Probabilities  = std::vector<double>;
using Subnet         = model::Subnet;

double ProbabilisticEstimate::combinations(size_t k, size_t n, std::vector<double> &prob) {
  
  double p = 0.0;

  for (size_t i = 0; i < (1ul << n); ++i) {
    std::bitset<32> bits(i);

    if (bits.count() == k) {
      double pComb = 1.0;

      for (size_t j = 0; j < n; ++j) {
        pComb = pComb * (bits[i] ? prob[j] : (1 - prob[j]));
      }

      p += pComb;
    }
  }
  
  return p;
}

double ProbabilisticEstimate::majEstimate(std::vector<double> &majProb, size_t nMajP) {

  double p = 0.0;

  for (size_t i = nMajP/2 + 1; i <= nMajP; i++) {
    p = p + combinations(i, nMajP, majProb);
  }

  return p;
}

double ProbabilisticEstimate::xorEstimate(std::vector<double> &xorProb, size_t nXorP) {

  double p = 0.0;
    
  for (size_t i = 1; i <= nXorP; i = i + 2) {
    p = p + combinations(i, nXorP, xorProb);
  }

  return p;
}

std::vector<double> ProbabilisticEstimate::probEstimator(const Subnet &subnet,
      const Probabilities &probabilities) {
  
  uint32_t lenInputProb = probabilities.size();
  size_t lenArrEst = subnet.size();
  std::vector<double> arrProbability;
  std::vector<double> cellEstimate;
  cellEstimate.resize(lenArrEst);;
  uint32_t k = 0;
  const auto cells = subnet.getEntries();
  
  for (size_t i = 0; i < lenArrEst; ++i) {
    double p = 1.0;
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
      const Probabilities &probabilities) {

  CellActivities cellEstimate = probEstimator(subnet, probabilities);
  size_t lenArrEst = subnet.size();

  for (size_t j = 0; j < lenArrEst; ++j) {
    double p = cellEstimate[j];
    cellEstimate[j] = 2 * p * (1 - p);
  }

  return SwitchActivity(cellEstimate);
}

} // namespace eda::gate::analyzer
