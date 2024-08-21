//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/net_printer.h"
#include "gate/optimizer/npnstatdb.h"

namespace eda::gate::optimizer {

NpnStatDatabase::ResultIterator NpnStatDatabase::get(const TT &tt, bool quiet) {
  auto config = kitty::exact_npn_canonization(tt);
  NpnTransformation t = utils::getTransformation(config);
  auto canonTT = utils::getTT(config);
  if (!quiet) {
    accessCounter[canonTT]++;
  }
  return ResultIterator(storage[canonTT], utils::inverse(t), info[canonTT]);
}

NpnStatDatabase::ResultIterator NpnStatDatabase::get(const TT &tt) {
  return get(tt, false);
}

NpnStatDatabase::ResultIterator NpnStatDatabase::getQuietly(const TT &tt) {
  return get(tt, true);
}

NpnStatDatabase::ResultIterator NpnStatDatabase::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt, false);
}

NpnStatDatabase::ResultIterator
NpnStatDatabase::getQuietly(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt, true);
}

NpnStatDatabase::NpnTransformation
NpnStatDatabase::push(const SubnetID &id,
                      const SubnetInfo &subnetInfo) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NpnTransformation t = utils::getTransformation(config);
  TT canonTT = utils::getTT(config);
  auto newId = utils::npnTransform(Subnet::get(id), t);
  storage[canonTT].push_back(newId);
  info[canonTT].push_back(subnetInfo);
  return t;
}

NpnStatDatabase::NpnTransformation
NpnStatDatabase::push(const SubnetID &id) {
  return push(id, SubnetInfo::makeEmpty());
}

void NpnStatDatabase::erase(const TT &tt) {
  storage.erase(tt);
  info.erase(tt);
}

const std::unordered_map<NpnStatDatabase::TT, uint64_t>
&NpnStatDatabase::getAccessCounter() {
  return accessCounter;
}

NpnStatDatabase NpnStatDatabase::importFrom(const std::string &filename) {
  std::ifstream in(filename);
  return NpnStatDatabaseSerializer().deserialize(in);
}

void NpnStatDatabase::exportTo(const std::string &filename) {
  std::ofstream out(filename);
  NpnStatDatabaseSerializer().serialize(out, *this);
}

void NpnStatDatabaseSerializer::serialize(std::ostream &out,
                                          const NpnStatDatabase &obj) {
  storageSerializer.serialize(out, obj.storage);
  infoSerializer.serialize(out, obj.info);
  acSerializer.serialize(out, obj.accessCounter);
}

NpnStatDatabase NpnStatDatabaseSerializer::deserialize(std::istream &in) {
  NpnStatDatabase result;
  result.storage = storageSerializer.deserialize(in);
  result.info = infoSerializer.deserialize(in);
  result.accessCounter = acSerializer.deserialize(in);
  return result;
}

void NpnStatDatabase::printDot(std::ostream &out, const TT &tt,
                        const std::string &name, const bool quiet) {
  NpnStatDatabase::ResultIterator iterator = get(tt, quiet);
  model::NetPrinter::getPrinter(model::NetPrinter::Format::DOT)
      .print(out, Subnet::get(iterator.get()), name);
}

void NpnStatDatabase::printInfo(std::ostream &out, const TT &tt, const bool quiet) {
  NpnStatDatabase::ResultIterator iterator = get(tt, quiet);
  Subnet &subnet1 = Subnet::get(iterator.get());
  printInfoSub(out, subnet1);
}

void NpnStatDatabase::printDotQuietly(std::ostream &out, const TT &tt,
                        const std::string &name) {
  printDot(out, tt, name, true);
}

void NpnStatDatabase::printDotFileQuietly(const TT &tt, const std::string &fileName,
                           const std::string &name) {
  printDotFile(tt, fileName, name, true);
}

void NpnStatDatabase::printInfoQuietly(std::ostream &out, const TT &tt) {
  printInfo(out, tt, true);
}

} // namespace eda::gate::optimizer
