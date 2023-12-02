//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/optimizer/resynthesis/reed_muller.h"
#include "gate/optimizer2/synthesizer.h"
#include "util/arith.h"

#include "kitty/kitty.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::optimizer2::resynthesis {

  using DinTruthTable = kitty::dynamic_truth_table;
  using Link = model::Subnet::Link;
  using LinkList = std::vector<Link>;
  using Polynomial = std::vector<uint64_t>;
  using SubnetID = model::SubnetID;

  /**
   * Class ReedMuller is created from Synthesizer.
   * It creates a logical graph and returns it as a SubnetID.
   * 
   * The implementation of the algorithm is based on the following article:
   * Harking B. Efficient algorithm for canonical Reed-Muller expansions 
   * of Boolean functions // IEE Proc., 1990. Vol. 137. №5. P. 366-370.
   */

  class ReedMuller : public Synthesizer<DinTruthTable> {
  public:

    /**
     * Transforms truth table for the function to subnet model 
     * using a Reed-Muller method.
     * 
     * maxArity is a parameter, that defines the maximum for Arity of every node in subnet.
     * By default it's set to Subnet::Cell::InPlaceLinks - 1, 
     * to use other value pass it as the second argument
     * 
     * maxArity has to be more than 2, if it's not => assert, however, if it's equal to 0
     * method doesn't create an assert, it uses the default value of maxArity
     * 
     */
    SubnetID synthesize(const DinTruthTable &func, uint16_t maxArity = -1) override;

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
    Polynomial getTT(const DinTruthTable &t);

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
    uint64_t apply(const Polynomial &func, const std::string &s);

  private:

    /**
     * Generates a characteristic function from a given truth table.
     *
     * char_func = func(0,...,0) ^ x1 & func(0,...,1) ^ ....
     *
     * @return characterisitic ponynomial of a given truth table
     */
    Polynomial charFromTruthTable(const DinTruthTable &t);

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
    Polynomial charFromFunction(Polynomial &func);
  };
} //namespace eda::gate::optimizer2::resynthesis
