//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npnstatdb.h"

namespace eda::gate::optimizer {

NPNStatDatabase::ResultIterator NPNStatDatabase::get(const TT &tt, bool quiet) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  auto canonTT = utils::getTT(config);
  if (!quiet) {
    accessCounter[canonTT]++;
  }
  return ResultIterator(storage[canonTT], utils::inverse(t), info[canonTT]);
}

NPNStatDatabase::ResultIterator NPNStatDatabase::get(const TT &tt) {
  return get(tt, false);
}

NPNStatDatabase::ResultIterator NPNStatDatabase::getQuietly(const TT &tt) {
  return get(tt, true);
}

NPNStatDatabase::ResultIterator NPNStatDatabase::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt, false);
}

NPNStatDatabase::ResultIterator
NPNStatDatabase::getQuietly(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt, true);
}

NPNStatDatabase::NPNTransformation
NPNStatDatabase::push(const SubnetID &id,
                      const SubnetInfo &subnetInfo) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  TT canonTT = utils::getTT(config);
  auto newId = utils::npnTransform(Subnet::get(id), t);
  storage[canonTT].push_back(newId);
  info[canonTT].push_back(subnetInfo);
  return t;
}

NPNStatDatabase::NPNTransformation
NPNStatDatabase::push(const SubnetID &id) {
  return push(id, SubnetInfo::makeEmpty());
}

void NPNStatDatabase::erase(const TT &tt) {
  storage.erase(tt);
  info.erase(tt);
}

const std::map<NPNStatDatabase::TT, uint64_t>
&NPNStatDatabase::getAccessCounter() {
  return accessCounter;
}

NPNStatDatabase NPNStatDatabase::importFrom(const std::string &filename) {
  std::ifstream in(filename);
  return NPNStatDatabaseSerializer().deserialize(in);
}

void NPNStatDatabase::exportTo(const std::string &filename) {
  std::ofstream out(filename);
  NPNStatDatabaseSerializer().serialize(out, *this);
}

void NPNStatDatabaseSerializer::serialize(std::ostream &out,
                                          const NPNStatDatabase &obj) {
  storageSerializer.serialize(out, obj.storage);
  infoSerializer.serialize(out, obj.info);
  acSerializer.serialize(out, obj.accessCounter);
}

NPNStatDatabase NPNStatDatabaseSerializer::deserialize(std::istream &in) {
  NPNStatDatabase result;
  result.storage = storageSerializer.deserialize(in);
  result.info = infoSerializer.deserialize(in);
  result.accessCounter = acSerializer.deserialize(in);
  return result;
}

} // namespace eda::gate::optimizer
