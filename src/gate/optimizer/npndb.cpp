//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npndb.h"

namespace eda::gate::optimizer {

using TT = NPNDatabase::TT;

NPNDatabase::ResultIterator NPNDatabase::get(const TT &tt) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  return ResultIterator(storage[utils::getTT(config)], utils::inverse(t));
}

NPNDatabase::ResultIterator NPNDatabase::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt);
}

NPNDatabase::NPNTransformation NPNDatabase::push(const SubnetID &id) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  auto newId = utils::npnTransform(Subnet::get(id), t);
  storage[std::get<0>(config)].push_back(newId);
  return t;
}

void NPNDatabase::erase(const TT &tt) {
  storage.erase(tt);
}

NPNDatabase NPNDatabase::importFrom(const std::string &filename) {
  std::ifstream in(filename);
  NPNDatabase result = NPNDatabaseSerializer().deserialize(in);
  return result;
}

void NPNDatabase::exportTo(const std::string &filename) const {
  std::ofstream out(filename);
  NPNDatabaseSerializer().serialize(out, *this);
}

void NPNDatabaseSerializer::serialize(std::ostream &out, const NPNDatabase &obj) {
  storageSerializer.serialize(out, obj.storage);
}

NPNDatabase NPNDatabaseSerializer::deserialize(std::istream &in) {
  NPNDatabase result;
  result.storage = storageSerializer.deserialize(in);
  return result;
}

} // namespace eda::gate::optimizer
