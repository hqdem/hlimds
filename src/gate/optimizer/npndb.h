//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "gate/optimizer/bgnet.h"

#include "kitty/kitty.hpp"

#include <bitset>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::optimizer {

/**
 * \brief Implements rewrite database which uses NPN matching to store nets.
 */
class NPNDatabase {
public:
  using InputPermutation = std::vector<uint8_t>;

  /**
   * Represents NPN transformation.
   * Includes input/output negation and input permutation.
   * First (n-1) bits of negationMask are responsible for inputs negation.
   * n'th bit of negationMask stores output negation.
   * Where *n* is number of inputs.
   */
  struct NPNTransformation {
    uint32_t negationMask;
    InputPermutation permutation;
  };

  using BoundGNetList = BoundGNet::BoundGNetList;
  using Gate = model::Gate;
  using GateSymbol = model::GateSymbol;
  using GetResult = std::tuple<BoundGNetList, NPNTransformation>;
  using TT = kitty::dynamic_truth_table;

  /**
   * \brief Builds truth table for bGNet.
   */
  static TT buildTT(const BoundGNet &bGNet);

  /**
   * \brief Inverses NPN transformation.
   */
  static NPNTransformation inverse(const NPNTransformation &t);

  /**
   * \brief Transforms bGNet by permuting inputs and negating inputs/outputs.
   * \param bGNet bound net to transform
   * \param t NPN-transformation to apply
   */
  static void npnTransformInplace(BoundGNet &bGNet, const NPNTransformation &t);

  /**
   * \brief Returns transformed clone of *bGNet* without changing *bGNet* itself.
   */
  static BoundGNet npnTransform(const BoundGNet &bGNet,
                                const NPNTransformation &t);

  /**
   * \brief Finds nets equivalent to representative function of *tt* NPN-class.
   * Return tuple which consists of BoundGNet list of nets implementing
   * function of class representative, negation mask and permutation to
   * apply to representative.
   */
  GetResult get(const TT &tt);
  GetResult get(const BoundGNet &bnet);

  /**
   * \brief Push *bnet*'s NPN representative function net in database.
   */
  NPNTransformation push(const BoundGNet &bnet);

  void erase(const TT &tt);

private:
  static TT applyGateFunc(const model::GateSymbol::Value func,
                          const std::vector<TT> &inputList);

  // Storage only contains NPN class representatives.
  std::map<TT, BoundGNetList> storage;
};

} // namespace eda::gate::optimizer
