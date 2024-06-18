//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/synthesizer.h"
#include "util/arith.h"

#include "kitty/kitty.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::optimizer::synthesis {

using Link = model::Subnet::Link;
using LinkList = std::vector<Link>;
using Polynomial = std::vector<uint64_t>;
using SubnetID = model::SubnetID;
using Polarization = std::vector<bool>;

/**
 * Function that creates SubnetID by a given Polynomial and its
 * Polarization with an arity equal to "maxArity".
 */
SubnetID createScheme(Polynomial &resultFunction, Polarization &polarization, 
                      uint64_t maxArity, uint64_t argNum);

/**
 * Class Zhegalkin is created from Synthesizer.
 * It creates a logical graph and returns it as a SubnetID.
 * 
 * The implementation of the algorithm is based on the following article:
 * Harking B. Efficient algorithm for canonical Reed-Muller expansions 
 * of Boolean functions // IEE Proc., 1990. Vol. 137. №5. P. 366-370.
 */

class ZhegalkinSynthesizer : public TruthTableSynthesizer {
public:

  using Synthesizer::synthesize;

  /**
   * Transforms truth table for the function to subnet model 
   * using a Reed-Muller method.
   * 
   * maxArity is a parameter, that defines the maximal arity of every node in subnet.
   * By default it's set to Subnet::Cell::InPlaceLinks, 
   * to use other value pass it as the second argument.
   * 
   * If the second argument passed to "synthesize" is more than Subnet::Cell::InPlaceLinks,
   * it's forced to be equal to Subnet::Cell::InPlaceLinks.
   * 
   * The maxArity param must be greater than 2.
   */
  SubnetID synthesize(const TruthTable &func, const TruthTable &,
                      uint16_t maxArity = -1) const override;

  /**
   * Creates a function, represented by a given truth table. 
   *
   * The result function is stored as a polynomial.
   *
   * Sample output: 
   *
   * kitty:create_from_binary_string(TT t, "10011100");
   *
   * getTT(t) = x_2 ^ x_3 ^ x_1 & x_3
   *
   * @return polynomical representation of truth table t
   */
  Polynomial getTT(const TruthTable &t) const;

  /**
   * Calculates the function of the variables given in the string.
   *
   * Before applying the function checks whether the size of a given binary
   * string is right (is the same, as the number of variables).
   *
   * If s.size() < num_vars -> add leading zeroes to make the padding right, 
   * then calculates the given function, using the string with leading zeroes. 
   * 
   * if s.size() > num_vars -> assert, nothing is calculated.
   * 
   * @return func(s)
   */
  uint64_t apply(const Polynomial &func, const std::string &s) const;

private:

  /**
   * Generates a characteristic function from a given truth table.
   *
   * char_func = func(0,...,0) ^ x1 & func(0,...,1) ^ ....
   *
   * @return characterisitic ponynomial of a given truth table
   */
  Polynomial charFromTruthTable(const TruthTable &t) const;

  /**
   * Generates a characteristic function from a given function.
   *
   * function is a polynomial func.
   *
   * char_func = func(0,...,0) ^ x1 & func(0,...,1) ^ ....
   *
   * @return characteristic polynomial of a given polynomial
   *
   */
  Polynomial charFromFunction(Polynomial &func) const;
};

} // namespace eda::gate::optimizer::synthesis
