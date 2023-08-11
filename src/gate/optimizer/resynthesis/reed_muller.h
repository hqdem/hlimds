//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "kitty/kitty.hpp"
#include <memory>
#include <string>
#include <vector>

#ifndef REED_MULLER_TEST_LOGICAL_FUNCTION_H
#define REED_MULLER_TEST_LOGICAL_FUNCTION_H

namespace eda::gate::optimizer::resynthesis {
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using GateSymbol = eda::gate::model::GateSymbol;
using Polynomial = std::vector<uint64_t>;
using DinTruthTable = kitty::dynamic_truth_table;

/**
 * Class ReedMuller creates a polynomial by a given truth table.
 *
 * P(x_1, x_2, ... , x_n) = a_0 ^ (a_1 & x_1) ^ (a_2 & x_2) ^ ... ^ (a_12 & x_1
 * & x_2) ^ ... where a_i is a coefficient (either 1 or 0)
 *
 * The implementation of the algorithm is based on the following article:
 * Harking B. Efficient algorithm for canonical Reed-Muller expansions 
 * of Boolean functions // IEE Proc., 1990. Vol. 137. №5. P. 366-370.
 */
class ReedMuller {

public:
  ReedMuller();

  virtual ~ReedMuller();

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
   * Creates a logical graph based on a kitty::dynamic_truth_table.
   *
   * @return logical graph equivalent to truth table t
   */
   std::shared_ptr<GNet> getGNet(const DinTruthTable &t);

  /**
   * Applies the given function to a given binary string.
   *
   * Before applying the function checks whether the size of a given binary
   * string is right (is the same, as the number of variables).
   *
   * If s.size() < num_vars -> add leading zeroes to make the padding right. 
   * if s.size() > num_vars -> assert.
   *
   * @return func({x1, x2, ... , xn})
   */
  uint64_t apply(const Polynomial &func, const std::string &s);

  /** 
   * Generates binary representation of a number with a padding.
   *        
   * Padding is the size of a result vector.
   *
   * Sample output: 
   *
   * to2(6, 4) = "1100", to2(4, 4) = "0100", to2(6,3) = "110"
   *
   * @return binary representation of a given number
   */
  std::string toBinString(int a, uint64_t padding);

private:
  /** 
   * Generates the vector, each element is a position of "1" in argument.
   *
   * Sample output: 
   *
   * popcnt(6) = {1,2} as 6 in binary is "110"
   *
   * @return vector of positions of ones in the binary representation of a given number
   */
  std::vector<int> popcnt(int a);

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
} // namespace eda::gate::optimizer::resynthesis

#endif // REED_MULLER_TEST_LOGICAL_FUNCTION_H
