//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "npndb.h"
#include "util/kitty_utils.h"

namespace eda::gate::optimizer {

NPNDatabase::ResultIterator NPNDatabase::get(const TT &tt) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  return ResultIterator(storage[utils::getTT(config)], utils::inverse(t));
}

NPNDatabase::ResultIterator NPNDatabase::get(const BoundGNet &bnet) {
  TT tt = utils::buildTT(bnet);
  return get(tt);
}

NPNDatabase::NPNTransformation NPNDatabase::push(const BoundGNet &bnet) {
  BoundGNet bnetClone = bnet.clone();
  TT tt = utils::buildTT(bnetClone);
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  utils::npnTransformInplace(bnetClone, t);
  storage[std::get<0>(config)].push_back(bnetClone);
  return t;
}

void NPNDatabase::erase(const TT &tt) {
  storage.erase(tt);
}

} // namespace eda::gate::optimizer

