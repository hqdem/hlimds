//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npndb.h"

namespace eda::gate::optimizer {

using TT = NPNDatabase2::TT;

NPNDatabase2::ResultIterator NPNDatabase2::get(const TT &tt) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  return ResultIterator(storage[utils::getTT(config)], utils::inverse(t));
}

NPNDatabase2::ResultIterator NPNDatabase2::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt);
}

NPNDatabase2::NPNTransformation NPNDatabase2::push(const SubnetID &id) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  auto newId = utils::npnTransform(Subnet::get(id), t);
  storage[std::get<0>(config)].push_back(newId);
  return t;
}

void NPNDatabase2::erase(const TT &tt) {
  storage.erase(tt);
}

} // namespace eda::gate::optimizer
