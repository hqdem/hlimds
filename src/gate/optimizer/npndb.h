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
#include "util/citerator.h"
#include "util/kitty_utils.h"

#include <sstream>
#include <vector>

namespace eda::gate::optimizer {

class NPNDBResultIterator : public ConstIterator<BoundGNet> {
public:
  using BoundGNetList = BoundGNet::BoundGNetList;
  using NPNTransformation = utils::NPNTransformation;

  NPNDBResultIterator(const BoundGNetList &l, const NPNTransformation &t) :
                      transformation(t), list(l), ind(0) { }
  NPNDBResultIterator(const BoundGNetList &&l, const NPNTransformation &t) :
                      transformation(t), list(std::move(l)), ind(0) { }

  bool isEnd() const override {
    return ind >= list.size();
  }

  bool next() override {
    if (isEnd()) {
      return false;
    }
    ind++;
    return isEnd();
  }

  BoundGNet get() const override {
    assert(!isEnd() && "End of the result.");
    return utils::npnTransform(list[ind], transformation);
  }

  size_t size() const override {
    return list.size();
  }

  operator bool() const override {
    return !isEnd();
  }

private:
  NPNTransformation transformation;
  BoundGNetList list;
  size_t ind;

};

/**
 * \brief Implements rewrite database which uses NPN matching to store nets.
 */
class NPNDatabase {
public:
  using BoundGNetList = BoundGNet::BoundGNetList;
  using Gate = model::Gate;
  using GateSymbol = model::GateSymbol;
  using ResultIterator = NPNDBResultIterator;
  using InputPermutation = std::vector<uint8_t>;
  using NPNTransformation = utils::NPNTransformation;
  using TT = kitty::dynamic_truth_table;

  /**
   * \brief Finds nets equivalent to representative function of *tt* NPN-class.
   * Return tuple which consists of BoundGNet list of nets implementing
   * function of class representative, negation mask and permutation to
   * apply to representative.
   */
  ResultIterator get(const TT &tt);
  ResultIterator get(const BoundGNet &bnet);

  /**
   * \brief Push *bnet*'s NPN representative function net in database.
   */
  NPNTransformation push(const BoundGNet &bnet);

  void erase(const TT &tt);

private:
  // Storage only contains NPN class representatives.
  std::map<TT, BoundGNetList> storage;
};

} // namespace eda::gate::optimizer

