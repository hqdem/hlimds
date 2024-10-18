//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/probabilistic_estimate.h"

namespace eda::gate::estimator {

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

void ProbabilityEstimator::estimateCell(const Builder &builder,
                                        Probabilities &probs,
                                        const size_t i,
                                        const Probabilities &inProbs) const {
  const auto &cell = builder.getCell(i);
  const LinkList &links = builder.getLinks(i);

  float p{1.0};

  if (cell.isIn()) {
    p = inProbs.empty() ? 0.5f : inProbs[i];
    probs[i] = p;
    return;
  }

  if (cell.isAnd()) {
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = links[j];
      auto cellEst = probs[link.idx];
      p = p * (link.inv ? (1 - cellEst) : cellEst);
    }
    probs[i] = p;
    return;
  }

  if (cell.isOr()) {
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = links[j];
      auto cellEst = probs[link.idx];
      p = p * (link.inv ? cellEst : (1 - cellEst));
    }
    p = 1 - p;
    probs[i] = p;
    return;
  }

  if (cell.isOne() || cell.isZero()) {
    probs[i] = cell.isOne() ? 1.0f : 0.0;
    return;
  }

  if (cell.isBuf()) {
    const auto link = links[0];
    p = link.inv ? (1 - probs[link.idx]) : probs[link.idx];
    probs[i] = p;
    return;
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
    probs[i] = p;
    return;
  }

  if (cell.isOut()) {
    const auto link = links[0];
    p = link.inv ? (1 - probs[link.idx]) : probs[link.idx];
    probs[i] = p;
    return;
  }

  probs[i] = 0.0;
}

Probabilities ProbabilityEstimator::estimateProbs(
    const Builder &builder,
    const Probabilities &probabilities) const {

  Probabilities probs(builder.getMaxIdx() + 1);

  for (auto it = builder.begin(); it != builder.end(); it.nextCell()) {
    estimateCell(builder, probs, *it, probabilities);
  }

  return probs;
}

void ProbabilityEstimator::estimate(const std::shared_ptr<Builder> &builder,
                                    const Probabilities &probabilities,
                                    SwitchActivity &result) const {

  Probabilities onState = estimateProbs(*(builder.get()), probabilities);
  Probabilities switching{onState};

  for (auto it = builder->begin(); it != builder->end(); ++it) {
    float p = switching[*it];
    switching[*it] = (2.f * p * (1.f - p));
  }

  result.setSwitchActivity(switching, onState);
}

} // namespace eda::gate::estimator
