//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/utils/subnet_truth_table.h"
#include "util/citerator.h"
#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"

#include <sstream>
#include <vector>

namespace eda::gate::optimizer {

class NPNDB2ResultIterator : public ConstIterator<model::SubnetID> {
public:
  using NPNTransformation = eda::utils::NPNTransformation;
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;

  NPNDB2ResultIterator(const SubnetIDList &l, const NPNTransformation &t) :
                       transformation(t), list(l), ind(0) { }
  NPNDB2ResultIterator(const SubnetIDList &&l, const NPNTransformation &t) :
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

  SubnetID get() const override {
    assert(!isEnd() && "End of the result.");
    return eda::utils::npnTransform(Subnet::get(list[ind]), transformation);
  }

  size_t size() const override {
    return list.size();
  }

  operator bool() const override {
    return !isEnd();
  }

private:
  NPNTransformation transformation;
  SubnetIDList list;
  size_t ind;
};

/**
 * \brief Implements rewrite database which uses NPN matching to store nets.
 */
class NPNDatabase2 {
public:
  using ResultIterator = NPNDB2ResultIterator;
  using NPNTransformation = utils::NPNTransformation;
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using TT = kitty::dynamic_truth_table;

  /**
   * \brief Finds nets equivalent to representative function of *tt* NPN-class.
   * Returns tuple which consists of subnet references list of nets implementing
   * function of class representative, negation mask and permutation to
   * apply to representative.
   */
  ResultIterator get(const TT &tt);
  ResultIterator get(const Subnet &subnet);

  /**
   * \brief Push *bnet*'s NPN representative function net in database.
   */
  NPNTransformation push(const SubnetID &id);

  void erase(const TT &tt);

private:
  // Storage only contains NPN class representatives.
  std::map<TT, SubnetIDList> storage;
};

} // namespace eda::gate::optimizer

