//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesis/zhegalkin.h"
#include "gate/optimizer/synthesizer.h"

#include "kitty/kitty.hpp"

#include <iostream>
#include <numeric>
#include <vector>

namespace eda::gate::optimizer::synthesis {

using Polarization = std::vector<bool>;
using Polynomial = std::vector<uint64_t>;
using PolarizedPolynomial = std::pair<Polynomial, std::vector<bool>>;
using SubnetObject = model::SubnetObject;
using TTable = std::vector<uint64_t>;

/**
* Metric functions:
* 1) numberOfTerms - calculates the number of terms in scheme. 
* 2) longestTerm - finds the length of a longest term.
*/
namespace {
  inline uint64_t termMetric(Polynomial &polynomial, bool countingLenght) {
    uint64_t result = 0;
    for (size_t i = 0; i < polynomial.size(); ++i) {
      if (polynomial[i]) {
        uint64_t popcount = 0;
        auto index = i;
        while (index) {
          popcount += index & 1;
          index >>= 1;
        }
        if (!countingLenght) {
          result = std::max(result, popcount);
        } else {
          result += popcount;
        }
      }    
    }
    return result;
  }
} // namespace

inline uint64_t numberOfTerms(Polynomial &polynomial) {
  return std::accumulate(polynomial.begin(), polynomial.end(), 0);
}

inline uint64_t longestTerm(Polynomial &polynomial) {
  return termMetric(polynomial, false);
}

inline uint64_t sumOfTerms(Polynomial &polynomial) {
  return termMetric(polynomial, true);
}

/**
* Class ReedMuller synthesizes scheme using ReedMuller
* method and makes this scheme optimal (by some metric, 
* metric can be defined by user).
* Algorithm and implementation is based on a following article:
* 
* Zakrevsky A. D., Toporov N. R. 
* "Polynomial realizations of partial boolean functions and systems".
* https://reallib.org/reader?file=1514696&pg=34.
*/
class ReedMuller : public TruthTableSynthesizer {

public:

  /**
   * Constructor that synthesizes the basic ReedMuller scheme.
   * Basic scheme is a scheme where all the terms are zero-polarized
   * (in other words - all the terms in scheme are not inverted)
   */
  ReedMuller(uint64_t (*metricFunction)(Polynomial &) = sumOfTerms);

  using Synthesizer::synthesize;

  /**
   * Generates a logical scheme by a truth table used in constructing
   * ReedMuller object. If you want to get an optimal (by some metric)
   * scheme, you need to pass this metric (as a pointer to a function)
   * as the first argument in this method. If no metric is passed method generates
   * a non-polarized scheme (scheme where all the inputs are positive). 
   */
  SubnetObject synthesize(const TruthTable &func, const TruthTable &,
                          uint16_t maxArity = -1) const override;

private:

  /**
   * Method used to perform polarization/changePolarity operations 
   */
  void polarityOperation(uint64_t index, bool rightShift) const;

  /**
   * Method constructs the optimal scheme and returns
   * it with array of polarizations. 
   * In this array element on position "i" defines whether
   * we have to polarize input of a scheme on position "i".
   */
  PolarizedPolynomial getOptimal() const;

  /**
   * This method goes through all the polarization of
   * a basic scheme and return the optimal one.
   * Currently the most optimal scheme - is a scheme
   * with the least number of terms.
   */
  uint64_t getPolarized() const;

  /**
   * Performs operation "polarize(i)" from the article. 
   */
  void polarize(uint64_t index) const;

  /**
   * Performs operation "change Polarity(i) from the article." 
   */
  void changePolarity(uint64_t index) const;

  /**
   * Method performs cyclic shift. Starting from C++ 20
   * this method can be removed (see the source code). 
   */
  void shift(uint64_t pos, bool right = false) const;
  
  /**
   * Number of inputs in scheme. 
   */
  mutable uint64_t variables;

  /**
   * ReedMuller polynomial (intermediate representation of scheme).
   */
  mutable Polynomial polynomial;

  /**
   * Pointer to a current metric function used for 
   * finding the optimal scheme for a given Truth Table 
   */
  mutable uint64_t (*currentMetricFunction)(Polynomial &);
};

} // namespace eda::gate::optimizer::synthesis
