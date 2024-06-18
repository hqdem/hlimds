//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "probabilistic_estimate.h"

namespace eda::gate::analyzer {

using Probabilities = ProbabilityEstimator::Probabilities;

float ProbabilityEstimator::combinations(
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

float ProbabilityEstimator::majEstimate(
    std::vector<float> &majProb, size_t nMajP) const {

  float p = 0.0;

  for (size_t i = nMajP/2 + 1; i <= nMajP; i++) {
    p = p + combinations(i, nMajP, majProb);
  }

  return p;
}

float ProbabilityEstimator::xorEstimate(
    std::vector<float> &xorProb, size_t nXorP) const {

  float p = 0.0;
    
  for (size_t i = 1; i <= nXorP; i = i + 2) {
    p = p + combinations(i, nXorP, xorProb);
  }

  return p;
}

float ProbabilityEstimator::estimateCell(Probabilities &probs,
                                         const LinkList &links,
                                         const Cell &cell,
                                         const size_t i,
                                         const Probabilities &inProbs) const {
  float p{1.0};

  if (cell.isIn()) {
    return inProbs.empty() ? 0.5f : inProbs[i];
  }

  if (cell.isAnd()) {
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = links[j];
      auto cellEst = probs[link.idx];
      p = p * (link.inv ? (1 - cellEst) : cellEst);
    }
    return p;
  }

  if (cell.isOr()) {
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = links[j];
      auto cellEst = probs[link.idx];
      p = p * (link.inv ? cellEst : (1 - cellEst));
    }
    p = 1 - p;
    return p;
  }

  if (cell.isOne()) {
    return 1.0;
  }

  if (cell.isZero()) {
    return 0.0;
  }

  if (cell.isBuf()) {
    const auto link = links[0];
    p = link.inv ? (1 - probs[link.idx]) : probs[link.idx];
    return p;
  }

  if (cell.isMaj() or cell.isXor()) {
    Probabilities arrProbability;
      
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = links[j];
      auto cellEst = probs[link.idx];
      auto cellPr = link.inv ? (1 - cellEst) : cellEst;
      arrProbability.push_back(cellPr);
    }

    size_t nArrProb = arrProbability.size();

    p = cell.isMaj() ? majEstimate(arrProbability, nArrProb)
        : xorEstimate(arrProbability, nArrProb);
    return p;
  }

  if (cell.isOut()) {
    const auto link = links[0];
    p = link.inv ? (1 - probs[link.idx]) : probs[link.idx];
    return p;
  }

  return 0.0;
}

Probabilities ProbabilityEstimator::estimateProbs(const Subnet &subnet,
    const Probabilities &probabilities) const {

  Probabilities probs(subnet.size());

  const auto &cells = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    probs[i] = estimateCell(probs, subnet.getLinks(i), cells[i].cell,
                            i, probabilities);
  }

  return probs;
}

Probabilities ProbabilityEstimator::estimateProbs(const SubnetBuilder &builder, 
    const Probabilities &probabilities) const {
  
  Probabilities probs(*(--builder.end()) + 1);

  for (auto it = builder.begin(); it != builder.end(); ++it) {
    probs[*it] = estimateCell(probs, builder.getLinks(*it), builder.getCell(*it),
                              *it, probabilities);
  }

  return probs;
}

SwitchActivity ProbabilityEstimator::estimate(const Subnet &subnet,
    const Probabilities &probabilities) const {

  Probabilities onState = estimateProbs(subnet, probabilities);
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
